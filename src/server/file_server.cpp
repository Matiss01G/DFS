#include "server/file_server.h"
#include "logging/logging.hpp"
#include <iostream>
#include <sstream>
#include <thread>

namespace dfs {
namespace server {

FileServer::FileServer(FileServerOpts opts) : opts_(std::move(opts)) {
  // Initialize logging for file server
  logging::init_logging("FileServer");

  LOG_INFO << "Initializing FileServer...";

  // Generate ID if none provided
  if (opts_.id.empty()) {
    opts_.id = crypto::generateId();
    LOG_DEBUG << "Generated new server ID: " << opts_.id;
  }

  // Initialize storage with options
  storage::StoreOpts storeOpts{.root = opts_.storageRoot,
                               .pathTransformFunc = opts_.pathTransformFunc};
  LOG_DEBUG << "Initializing storage with root directory: "
            << opts_.storageRoot;
  store_ = std::make_unique<storage::Store>(storeOpts);

  // Set transport callback
  if (opts_.transport) {
    LOG_DEBUG << "Setting up transport callbacks";
    opts_.transport->setOnPeer([this](std::shared_ptr<network::Peer> peer) {
      return this->OnPeer(peer);
    });
  } else {
    LOG_WARN << "No transport provided in options";
  }

  LOG_INFO << "FileServer initialization complete";
}

FileServer::~FileServer() {
  LOG_DEBUG << "FileServer destructor called";
  Stop();
}

bool FileServer::Start() {
  if (running_) {
    LOG_WARN << "Attempted to start already running FileServer";
    return false;
  }

  LOG_INFO << "[" << opts_.transport->Addr() << "] Starting fileserver...";

  // Start transport
  if (!opts_.transport->ListenAndAccept()) {
    LOG_ERROR << "Failed to start transport on address: "
              << opts_.transport->Addr();
    return false;
  }

  // Verify RPC channel is available before starting message loop
  auto rpcChan = opts_.transport->Consume();
  if (!rpcChan) {
    LOG_ERROR << "Failed to initialize RPC channel";
    return false;
  }

  running_ = true;
  LOG_DEBUG << "Server state changed to running";

  // Start message processing loop
  loopThread_ = std::make_unique<std::thread>([this]() {
    LOG_DEBUG << "Message processing loop started";
    try {
      this->messageLoop();
    } catch (const std::exception &e) {
      LOG_ERROR << "Message loop error: " << e.what();
      LOG_ERROR << "Stack trace: " << e.what();
      running_ = false;
    }
  });

  // Connect to bootstrap nodes
  bootstrapNetwork();

  LOG_INFO << "Server startup complete";
  return true;
}

void FileServer::Stop() {
  LOG_DEBUG << "[" << opts_.transport->Addr() << "] Stop called";
  if (!running_) {
    LOG_DEBUG << "Server already stopped";
    return;
  }

  running_ = false;
  LOG_DEBUG << "Server state changed to stopped";

  if (loopThread_ && loopThread_->joinable()) {
    LOG_DEBUG << "[" << opts_.transport->Addr()
              << "] Joining message loop thread";
    loopThread_->join();
    LOG_DEBUG << "[" << opts_.transport->Addr()
              << "] Message loop thread joined";
  }

  {
    std::lock_guard<std::mutex> lock(peerLock_);
    LOG_INFO << "[" << opts_.transport->Addr() << "] Clearing " << peers_.size()
             << " peers";
    peers_.clear();
    LOG_DEBUG << "[" << opts_.transport->Addr() << "] Peers cleared";
  }

  if (opts_.transport) {
    LOG_DEBUG << "[" << opts_.transport->Addr() << "] Closing transport";
    opts_.transport->Close();
    LOG_DEBUG << "[" << opts_.transport->Addr() << "] Transport closed";
  }
  LOG_INFO << "[" << opts_.transport->Addr() << "] Stop complete";
}

bool FileServer::Store(const std::string &key, std::istream &data) {
  std::string hashedKey = crypto::hashKey(key);
  LOG_INFO << "[" << opts_.transport->Addr()
           << "] Storing file with key: " << key
           << " and hashed key: " << hashedKey;

  // Log initial stream state
  LOG_DEBUG << "Input stream state - good: " << data.good()
            << ", eof: " << data.eof() << ", fail: " << data.fail();

  // Store original data
  auto fileBuffer = std::make_shared<std::stringstream>();
  *fileBuffer << data.rdbuf();

  LOG_DEBUG << "Buffered data size: " << fileBuffer->tellp() << " bytes";

  int64_t size = store_->HashAndWrite(opts_.id, key, *fileBuffer);
  if (size < 0) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to write file to store. Error code: " << size;
    return false;
  }

  LOG_DEBUG << "Successfully wrote " << size << " bytes to local store";

  Message msg{.type = Message::Type::StoreFile,
              .payload = {
                  {"id", opts_.id},
                  {"key", hashedKey},
                  {"size", size + 16} // +16 for IV
              }};

  LOG_DEBUG << "Broadcasting store message to peers";
  if (!broadcast(msg)) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to broadcast store message";
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // Collect active peers
  std::vector<network::Peer *> activePeers;
  {
    std::lock_guard<std::mutex> lock(peerLock_);
    for (const auto &[_, peer] : peers_) {
      activePeers.push_back(peer.get());
      LOG_DEBUG << "Added peer " << peer->RemoteAddr() << " for replication";
    }
  }

  LOG_DEBUG << "Starting file replication to " << activePeers.size()
            << " peers";

  // Signal stream start
  for (auto peer : activePeers) {
    LOG_DEBUG << "Sending stream start signal to " << peer->RemoteAddr();
    peer->SendMessageType({network::MessageType::IncomingStream});
  }

  // Encrypt data
  boost::asio::streambuf encryptedBuffer;
  std::ostream encryptedStream(&encryptedBuffer);

  fileBuffer->seekg(0);
  LOG_DEBUG << "Starting encryption of " << fileBuffer->tellg() << " bytes";

  auto bytesProcessed =
      crypto::copyEncrypt(opts_.encKey, *fileBuffer, encryptedStream);
  if (bytesProcessed < 0) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to encrypt file data";
    LOG_DEBUG << "Encryption error code: " << bytesProcessed;
    return false;
  }

  LOG_DEBUG << "Successfully encrypted " << bytesProcessed << " bytes";

  // Send encrypted data to each peer
  size_t successfulReplications = 0;
  for (auto peer : activePeers) {
    LOG_DEBUG << "Sending encrypted data to peer " << peer->RemoteAddr();
    if (peer->WriteStream(encryptedBuffer)) {
      successfulReplications++;
      LOG_DEBUG << "Successfully replicated to " << peer->RemoteAddr();
    } else {
      LOG_ERROR << "Failed to replicate to " << peer->RemoteAddr();
    }
  }

  LOG_INFO << "File replication complete. Successfully replicated to "
           << successfulReplications << "/" << activePeers.size() << " peers";

  return true;
}

std::unique_ptr<std::istream> FileServer::Get(const std::string &key) {
  LOG_DEBUG << "[" << opts_.transport->Addr()
            << "] Get request for key: " << key;

  // First check if we have it locally
  if (store_->Has(opts_.id, key)) {
    LOG_INFO << "[" << opts_.transport->Addr() << "] Serving file (" << key
             << ") from local disk";
    auto [size, stream] = store_->Read(opts_.id, key);
    if (!stream) {
      LOG_ERROR << "Failed to read file from local store";
      return nullptr;
    }
    LOG_DEBUG << "Successfully read " << size << " bytes from local store";
    return std::move(stream);
  }

  LOG_INFO << "[" << opts_.transport->Addr() << "] Don't have file (" << key
           << ") locally, fetching from network...";

  // Request file from network
  Message msg{.type = Message::Type::GetFile,
              .payload = {{"id", opts_.id}, {"key", crypto::hashKey(key)}}};

  // Broadcast request to network
  LOG_DEBUG << "Broadcasting get request to network";
  if (!broadcast(msg)) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to broadcast get request";
    return nullptr;
  }

  LOG_DEBUG << "Waiting for network responses";
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Check again locally - if a peer had the file, it should now be replicated
  // locally
  if (store_->Has(opts_.id, key)) {
    LOG_INFO << "File received and stored locally, reading from disk";
    auto [size, stream] = store_->Read(opts_.id, key);
    if (!stream) {
      LOG_ERROR << "Failed to read replicated file from local store";
      return nullptr;
    }
    LOG_DEBUG << "Successfully read " << size << " bytes from local store";
    return std::move(stream);
  }

  LOG_WARN << "[" << opts_.transport->Addr()
           << "] File not found in network: " << key;
  return nullptr;
}

bool FileServer::OnPeer(std::shared_ptr<network::Peer> peer) {
  LOG_DEBUG << "[" << opts_.transport->Addr() << "] OnPeer called for "
            << peer->RemoteAddr();

  std::lock_guard<std::mutex> lock(peerLock_);
  LOG_DEBUG << "[" << opts_.transport->Addr() << "] Adding peer "
            << peer->RemoteAddr() << " to peers map";
  peers_[peer->RemoteAddr()] = peer;
  LOG_INFO << "[" << opts_.transport->Addr() << "] Connected with remote "
           << peer->RemoteAddr();

  LOG_DEBUG << "[" << opts_.transport->Addr()
            << "] Total peers: " << peers_.size();
  return true;
}

void FileServer::handleMessage(const std::string &from, const Message &msg) {
  LOG_DEBUG << "Handling message from " << from << " of type "
            << static_cast<int>(msg.type);

  try {
    switch (msg.type) {
    case Message::Type::StoreFile:
      LOG_DEBUG << "Processing StoreFile message";
      handleStoreFile(from, msg.payload);
      break;
    case Message::Type::GetFile:
      LOG_DEBUG << "Processing GetFile message";
      handleGetFile(from, msg.payload);
      break;
    default:
      LOG_WARN << "Unknown message type received: "
               << static_cast<int>(msg.type);
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "Error handling message: " << e.what();
    LOG_DEBUG << "Message sender: " << from;
    LOG_DEBUG << "Message type: " << static_cast<int>(msg.type);
  }
}

bool FileServer::handleStoreFile(const std::string &from,
                                 const nlohmann::json &payload) {
  LOG_DEBUG << "Handling store file request from " << from;

  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    LOG_ERROR << "[" << opts_.transport->Addr() << "] Peer not found: " << from;
    return false;
  }

  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();
  auto size = payload["size"].get<int64_t>();

  LOG_DEBUG << "[" << opts_.transport->Addr()
            << "] Store file details - ID: " << id << ", Key: " << key
            << ", Size: " << size << " bytes";

  // Stream to hold encrypted data from peer
  std::stringstream encryptedData;
  // Stream for decrypted output
  std::stringstream decryptedData;

  LOG_DEBUG << "Reading encrypted data from network";
  peer->second->ReadStream(encryptedData, size);

  LOG_DEBUG << "Starting decryption of " << encryptedData.tellp() << " bytes";
  auto bytesProcessed =
      crypto::copyDecrypt(opts_.encKey, encryptedData, decryptedData);
  if (bytesProcessed < 0) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to decrypt data from peer";
    LOG_DEBUG << "Decryption error code: " << bytesProcessed;
    return false;
  }

  LOG_DEBUG << "Successfully decrypted " << bytesProcessed << " bytes";

  // Write decrypted data to store
  auto result = store_->Write(id, key, decryptedData);
  if (result < 0) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to write data to store";
    LOG_DEBUG << "Store error code: " << result;
    return false;
  }

  LOG_INFO << "[" << opts_.transport->Addr() << "] Written " << result
           << " bytes to disk";

  peer->second->CloseStream();
  LOG_DEBUG << "Stream closed for peer " << from;
  return true;
}

bool FileServer::handleGetFile(const std::string &from,
                               const nlohmann::json &payload) {
  LOG_DEBUG << "Handling get file request from " << from;

  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();

  LOG_DEBUG << "[" << opts_.transport->Addr()
            << "] Get file details - ID: " << id << ", Key: " << key;

  if (!store_->Has(id, key)) {
    LOG_WARN << "[" << opts_.transport->Addr() << "] Requested file (" << key
             << ") not found in local store";
    return false;
  }

  LOG_INFO << "[" << opts_.transport->Addr() << "] Serving file (" << key
           << ") over the network";

  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    LOG_ERROR << "[" << opts_.transport->Addr() << "] Peer not found: " << from;
    return false;
  }

  auto [fileSize, stream] = store_->Read(id, key);
  if (!stream) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to read file from store";
    return false;
  }

  LOG_DEBUG << "Successfully read " << fileSize << " bytes from local store";

  // Send message type first
  LOG_DEBUG << "Sending stream start signal";
  peer->second->SendMessageType({network::MessageType::IncomingStream});

  // Convert fileSize to network byte order and send
  LOG_DEBUG << "Sending file size: " << fileSize << " bytes";
  uint64_t networkFileSize = htobe64(fileSize);

  // Convert int64_t to 8 bytes and send
  peer->second->Send(std::vector<uint8_t>{
      reinterpret_cast<const uint8_t *>(&fileSize),
      reinterpret_cast<const uint8_t *>(&fileSize) + sizeof(fileSize)});

  // Create output stream wrapper for peer socket
  boost::asio::streambuf encryptedBuffer;
  std::ostream encryptedStream(&encryptedBuffer);

  LOG_DEBUG << "Starting file encryption";
  // Encrypt and send the file
  auto bytesProcessed =
      crypto::copyEncrypt(opts_.encKey, *stream, encryptedStream);
  if (bytesProcessed < 0) {
    LOG_ERROR << "[" << opts_.transport->Addr()
              << "] Failed to encrypt file for transfer";
    LOG_DEBUG << "Encryption error code: " << bytesProcessed;
    return false;
  }

  LOG_DEBUG << "Successfully encrypted " << bytesProcessed << " bytes";

  // Write the encrypted data to socket
  LOG_DEBUG << "Sending encrypted data to peer";
  if (!peer->second->WriteStream(encryptedBuffer)) {
    LOG_ERROR << "Failed to write encrypted data to peer socket";
    return false;
  }

  LOG_INFO << "[" << opts_.transport->Addr()
           << "] Successfully sent file to peer " << from;
  return true;
}

bool FileServer::broadcast(const Message &msg) {
  std::lock_guard<std::mutex> lock(peerLock_);
  LOG_DEBUG << "Broadcasting message to " << peers_.size() << " peers";

  for (const auto &[addr, peer] : peers_) {
    LOG_DEBUG << "Preparing to send to peer: " << addr;

    // Serialize message
    std::stringstream ss;
    nlohmann::json j = {{"type", static_cast<int>(msg.type)},
                        {"payload", msg.payload}};
    ss << j.dump();

    // Send as regular message
    LOG_DEBUG << "Sending message type signal to " << addr;
    if (!peer->SendMessageType({network::MessageType::IncomingMessage})) {
      LOG_ERROR << "Failed to send message type to peer: " << addr;
      continue;
    }

    std::string str = ss.str();
    std::vector<uint8_t> data(str.begin(), str.end());

    // Send size in network byte order
    uint32_t payloadSize = htonl(static_cast<uint32_t>(data.size()));
    LOG_DEBUG << "Sending payload size (" << data.size() << " bytes) to "
              << addr;

    if (!peer->Send(std::vector<uint8_t>(
            reinterpret_cast<uint8_t *>(&payloadSize),
            reinterpret_cast<uint8_t *>(&payloadSize) + sizeof(payloadSize)))) {
      LOG_ERROR << "Failed to send payload size to peer: " << addr;
      continue;
    }

    // Send payload
    LOG_DEBUG << "Sending payload to " << addr;
    if (!peer->Send(data)) {
      LOG_ERROR << "Failed to send payload to peer: " << addr;
      continue;
    }

    LOG_DEBUG << "Successfully sent message to peer: " << addr;
  }

  LOG_INFO << "Broadcast complete";
  return true;
}

void FileServer::bootstrapNetwork() {
  LOG_INFO << "Starting network bootstrap process";

  for (const auto &addr : opts_.bootstrapNodes) {
    if (addr.empty()) {
      LOG_DEBUG << "Skipping empty bootstrap address";
      continue;
    }

    std::thread([this, addr]() {
      LOG_INFO << "[" << opts_.transport->Addr()
               << "] Attempting to connect with remote " << addr;
      if (!opts_.transport->Dial(addr)) {
        LOG_ERROR << "[" << opts_.transport->Addr() << "] Dial error: " << addr;
      } else {
        LOG_INFO << "Successfully connected to bootstrap node: " << addr;
      }
    }).detach();
  }

  LOG_INFO << "Bootstrap process initiated for " << opts_.bootstrapNodes.size()
           << " nodes";
}

void FileServer::messageLoop() {
  auto rpcChan = opts_.transport->Consume();

  if (!rpcChan) {
    LOG_ERROR << "Failed to get message channel from transport";
    return;
  }

  LOG_INFO << "Message loop started";
  size_t messageCount = 0;
  auto lastStatsTime = std::chrono::steady_clock::now();

  while (running_) {
    auto rpc = rpcChan->TryReceive();
    if (!rpc) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    messageCount++;
    auto now = std::chrono::steady_clock::now();
    if (now - lastStatsTime > std::chrono::seconds(60)) {
      LOG_INFO
          << "Message processing stats - Messages processed in last minute: "
          << messageCount;
      messageCount = 0;
      lastStatsTime = now;
    }

    try {
      LOG_DEBUG << "Processing message from: " << rpc->from()
                << " size: " << rpc->payload().size() << " bytes";

      auto j = nlohmann::json::parse(rpc->payload());
      Message msg{.type = static_cast<Message::Type>(j["type"].get<int>()),
                  .payload = j["payload"]};

      LOG_DEBUG << "Decoded message type: " << static_cast<int>(msg.type);
      handleMessage(rpc->from(), msg);

    } catch (const std::exception &e) {
      LOG_ERROR << "Error decoding message: " << e.what();
      LOG_DEBUG << "Raw message payload size: " << rpc->payload().size();
      LOG_DEBUG << "From peer: " << rpc->from();
    }
  }

  LOG_INFO << "Message loop terminated";
}

} // namespace server
} // namespace dfs