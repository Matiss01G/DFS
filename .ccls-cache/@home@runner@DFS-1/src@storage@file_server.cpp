#include "server/file_server.h"
#include <iostream>
#include <sstream>
#include <thread>

namespace dfs {
namespace server {

FileServer::FileServer(FileServerOpts opts) : opts_(std::move(opts)) {
  // Generate ID if none provided
  if (opts_.id.empty()) {
    opts_.id = crypto::generateId();
  }

  // Initialize storage with options
  storage::StoreOpts storeOpts{.root = opts_.storageRoot,
                               .pathTransformFunc = opts_.pathTransformFunc};
  store_ = std::make_unique<storage::Store>(storeOpts);

  // Set transport callback
  if (opts_.transport) {
    opts_.transport->setOnPeer(
        [this](network::Peer &peer) { return this->OnPeer(peer); });
  }
}

FileServer::~FileServer() { Stop(); }

// --------------------
// Startup and shutdown
// --------------------

// bootstrap fileserver
bool FileServer::Start() {
  if (running_)
    return false;

  std::cout << "[" << opts_.transport->Addr() << "] starting fileserver...\n";

  // Start transport
  if (!opts_.transport->ListenAndAccept()) {
    std::cerr << "Failed to start transport\n";
    return false;
  }

  // Verify RPC channel is available before starting message loop
  auto rpcChan = opts_.transport->Consume();
  if (!rpcChan) {
    std::cerr << "Failed to initialize RPC channel\n";
    return false;
  }

  running_ = true;

  // Start message processing loop
  loopThread_ = std::make_unique<std::thread>([this]() {
    try {
      this->messageLoop();
    } catch (const std::exception &e) {
      std::cerr << "Message loop error: " << e.what() << std::endl;
      running_ = false;
    }
  });
  
  // Connect to bootstrap nodes
  bootstrapNetwork();

  return true;
}

// shut down file server
void FileServer::Stop() {
  if (!running_)
    return;

  running_ = false;

  if (loopThread_ && loopThread_->joinable()) {
    loopThread_->join();
  }

  if (opts_.transport) {
    opts_.transport->Close();
  }
}

// ----------------------------
// Initiator methods: store/get
// ----------------------------

// Stores file in DFS
bool FileServer::Store(const std::string &key, std::istream &data) {
  // Store original data
  auto fileBuffer = std::make_shared<std::stringstream>();
  *fileBuffer << data.rdbuf();

  int64_t size = store_->Write(opts_.id, key, *fileBuffer);
  if (size < 0) {
    return false;
  }

  Message msg{.type = Message::Type::StoreFile,
              .payload = {
                  {"id", opts_.id},
                  {"key", crypto::hashKey(key)},
                  {"size", size + 16} // +16 for IV
              }};

  if (!broadcast(msg)) {
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // Collect active peers
  std::vector<network::Peer *> activePeers;
  {
    std::lock_guard<std::mutex> lock(peerLock_);
    for (const auto &[_, peer] : peers_) {
      activePeers.push_back(peer.get());
    }
  }

  // Signal stream start
  for (auto peer : activePeers) {
    peer->SendMessageType({network::MessageType::IncomingStream});
  }

  // Encrypt data
  boost::asio::streambuf encryptedBuffer;
  std::ostream encryptedStream(&encryptedBuffer);

  fileBuffer->seekg(0);
  auto bytesProcessed =
      crypto::copyEncrypt(opts_.encKey, *fileBuffer, encryptedStream);
  if (bytesProcessed < 0) {
    return false;
  }

  // Send encrypted data to each peer
  for (auto peer : activePeers) {
    peer->WriteStream(encryptedBuffer);
  }

  return true;
}

// Retrieves file from DFS
std::unique_ptr<std::istream> FileServer::Get(const std::string &key) {
  // First check if we have it locally
  if (store_->Has(opts_.id, key)) {
    std::cout << "[" << opts_.transport->Addr() << "] serving file (" << key
              << ") from local disk\n";
    auto [size, stream] = store_->Read(opts_.id, key);
    return std::move(stream);
  }

  std::cout << "[" << opts_.transport->Addr() << "] don't have file (" << key
            << ") locally, fetching from network...\n";

  // Request file from network
  Message msg{.type = Message::Type::GetFile,
              .payload = {{"id", opts_.id}, {"key", crypto::hashKey(key)}}};

  // Broadcast request to network
  if (!broadcast(msg)) {
    return nullptr;
  }

  // Wait briefly for responses & replication to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Check again locally - if a peer had the file, it should now be replicated
  // locally
  if (store_->Has(opts_.id, key)) {
    auto [size, stream] = store_->Read(opts_.id, key);
    return std::move(stream);
  }

  return nullptr;
}

bool FileServer::OnPeer(network::Peer &peer) {
  std::lock_guard<std::mutex> lock(peerLock_);
  peers_[peer.RemoteAddr()] = std::shared_ptr<network::Peer>(&peer);
  std::cout << "Connected with remote " << peer.RemoteAddr() << std::endl;
  return true;
}

// simple switch stetement select the correct message handler
// based on message type
void FileServer::handleMessage(const std::string &from, const Message &msg) {
  try {
    switch (msg.type) {
    case Message::Type::StoreFile:
      handleStoreFile(from, msg.payload);
      break;
    case Message::Type::GetFile:
      handleGetFile(from, msg.payload);

      break;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error handling message: " << e.what() << std::endl;
  }
}

// ------------------------------------------------
// Responder Methods: handleStoreFile/handleGetFile
// ------------------------------------------------

// Receives encrypted file stream from a peer and stores it locally
bool FileServer::handleStoreFile(const std::string &from,
                                 const nlohmann::json &payload) {
  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    return false;
  }

  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();
  auto size = payload["size"].get<int64_t>();

  // Stream to hold encrypted data from peer
  std::stringstream encryptedData;
  // Stream for decrypted output
  std::stringstream decryptedData;

  // Read encrypted data from network
  peer->second->ReadStream(encryptedData, size);

  // Decrypt data from socket to stream
  auto bytesProcessed =
      crypto::copyDecrypt(opts_.encKey, encryptedData, decryptedData);
  if (bytesProcessed < 0) {
    return false;
  }

  // Write decrypted data to store
  auto result = store_->Write(id, key, decryptedData);
  if (result < 0) {
    return false;
  }

  std::cout << "[" << opts_.transport->Addr() << "] written " << result
            << " bytes to disk\n";

  peer->second->CloseStream();
  return true;
}

// Responds to a file request by sending file to requesting peer if we have it
// locally
bool FileServer::handleGetFile(const std::string &from,
                               const nlohmann::json &payload) {
  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();

  if (!store_->Has(id, key)) {
    std::cerr << "[" << opts_.transport->Addr() << "] need to serve file ("
              << key << ") but it does not exist on disk\n";
    return false;
  }

  std::cout << "[" << opts_.transport->Addr() << "] serving file (" << key
            << ") over the network\n";

  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    return false;
  }

  auto [fileSize, stream] = store_->Read(id, key);
  if (!stream) {
    return false;
  }

  // Send message type first
  peer->second->SendMessageType({network::MessageType::IncomingStream});

  // Convert int64_t to 8 bytes and send
  peer->second->Send(std::vector<uint8_t>{
      reinterpret_cast<const uint8_t *>(&fileSize),
      reinterpret_cast<const uint8_t *>(&fileSize) + sizeof(fileSize)});

  // Create output stream wrapper for peer socket
  boost::asio::streambuf encryptedBuffer;
  std::ostream encryptedStream(&encryptedBuffer);

  // Encrypt and send the file
  auto bytesProcessed =
      crypto::copyEncrypt(opts_.encKey, *stream, encryptedStream);
  if (bytesProcessed < 0) {
    return false;
  }

  // Write the encrypted data to socket
  peer->second->WriteStream(encryptedBuffer);

  return true;
}

// ---------------------

// Sends a message to all connected peers
bool FileServer::broadcast(const Message &msg) {
  std::lock_guard<std::mutex> lock(peerLock_);
  for (const auto &[_, peer] : peers_) {
    // Serialize message
    std::stringstream ss;
    nlohmann::json j = {{"type", static_cast<int>(msg.type)},
                        {"payload", msg.payload}};
    ss << j.dump();

    // Send as regular message
    peer->SendMessageType({network::MessageType::IncomingMessage});

    std::string str = ss.str();
    std::vector<uint8_t> data(str.begin(), str.end());
    if (!peer->Send(data)) {
      return false;
    }
  }
  return true;
}

// ---------------------

// Initializes network by connecting to set of known bootstrap nodes
void FileServer::bootstrapNetwork() {
  for (const auto &addr : opts_.bootstrapNodes) {
    if (addr.empty())
      continue;

    std::thread([this, addr]() {
      std::cout << "[" << opts_.transport->Addr()
                << "] attempting to connect with remote " << addr << "\n";
      if (!opts_.transport->Dial(addr)) {
        std::cerr << "dial error: " << addr << std::endl;
      }
    }).detach();
  }
}

// Main loop
void FileServer::messageLoop() {
  auto rpcChan = opts_.transport->Consume();

  if (!rpcChan) {
    std::cerr << "Failed to get message channel from transport\n";
    return;
  }

  while (running_) {
    auto rpc = rpcChan->TryReceive();
    if (!rpc) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    try {
      auto j = nlohmann::json::parse(rpc->payload());
      Message msg{.type = static_cast<Message::Type>(j["type"].get<int>()),
                  .payload = j["payload"]};
      handleMessage(rpc->from(), msg);
    } catch (const std::exception &e) {
      std::cerr << "Error decoding message: " << e.what() << std::endl;
    }
  }
}

} // namespace server
} // namespace dfs