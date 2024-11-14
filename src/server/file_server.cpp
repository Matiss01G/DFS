#include "server/server.h"
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
    opts_.transport->OnPeer = [this](network::Peer &peer) {
      return this->OnPeer(peer);
    };
  }
}

FileServer::~FileServer() { Stop(); }

bool FileServer::Start() {
  if (running_)
    return false;

  std::cout << "[" << opts_.transport->Addr() << "] starting fileserver...\n";

  // Start transport
  if (!opts_.transport->ListenAndAccept()) {
    std::cerr << "Failed to start transport\n";
    return false;
  }

  running_ = true;

  // Start message processing loop
  loopThread_ =
      std::make_unique<std::thread>([this]() { this->messageLoop(); });

  // Connect to bootstrap nodes
  bootstrapNetwork();

  return true;
}

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

bool FileServer::Store(const std::string &key, std::istream &data) {
  // First store locally
  auto fileBuffer = std::make_shared<std::stringstream>();
  *fileBuffer << data.rdbuf(); // Copy input stream

  int64_t size = store_->Write(opts_.id, key, *fileBuffer);
  if (size < 0) {
    return false;
  }

  // Broadcast store message to peers
  Message msg{.type = Message::Type::StoreFile,
              .payload = {
                  {"id", opts_.id},
                  {"key", crypto::hashKey(key)},
                  {"size", size + 16} // Add 16 for IV
              }};

  if (!broadcast(msg)) {
    return false;
  }

  // Wait briefly for peers to prepare
  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // Stream encrypted file to all peers
  fileBuffer->seekg(0); // Reset to start
  std::vector<network::Peer *> activePeers;
  {
    std::lock_guard<std::mutex> lock(peerLock_);
    for (const auto &[_, peer] : peers_) {
      activePeers.push_back(peer.get());
    }
  }

  // Send stream indicator
  for (auto peer : activePeers) {
    peer->Send({network::MessageType::IncomingStream});
  }

  // Encrypt and send file data
  auto streamInfo = crypto::createEncryptStream(opts_.encKey);
  std::vector<uint8_t> buffer(8192);
  while (fileBuffer->read(reinterpret_cast<char *>(buffer.data()),
                          buffer.size())) {
    size_t bytesRead = fileBuffer->gcount();
    streamInfo.stream->ProcessData(buffer.data(), buffer.data(), bytesRead);

    for (auto peer : activePeers) {
      peer->Send(buffer);
    }
  }

  return true;
}

std::unique_ptr<std::istream> FileServer::Get(const std::string &key) {
  // Check if we have it locally
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

  if (!broadcast(msg)) {
    return nullptr;
  }

  // Wait briefly for responses
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Read from first responding peer
  std::lock_guard<std::mutex> lock(peerLock_);
  for (const auto &[_, peer] : peers_) {
    // Read file size
    int64_t fileSize;
    if (peer->Read(&fileSize, sizeof(fileSize)) != sizeof(fileSize)) {
      continue;
    }

    // Write decrypted data to local storage
    auto result =
        store_->WriteDecrypt(opts_.encKey, opts_.id, key, *peer, fileSize);
    if (result < 0) {
      continue;
    }

    peer->CloseStream();

    // Return stream from local storage
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

bool FileServer::handleStoreFile(const std::string &from,
                                 const nlohmann::json &payload) {
  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    return false;
  }

  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();
  auto size = payload["size"].get<int64_t>();

  // Store encrypted data from peer
  auto result = store_->Write(id, key, *peer->second);
  if (result < 0) {
    return false;
  }

  std::cout << "[" << opts_.transport->Addr() << "] written " << result
            << " bytes to disk\n";

  peer->second->CloseStream();
  return true;
}

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

  // Send stream indicator and file size
  peer->second->Send({network::MessageType::IncomingStream});
  peer->second->Send({fileSize});

  // Copy file data to peer
  std::vector<uint8_t> buffer(8192);
  while (stream->read(reinterpret_cast<char *>(buffer.data()), buffer.size())) {
    size_t bytesRead = stream->gcount();
    peer->second->Send({buffer.data(), buffer.data() + bytesRead});
  }

  return true;
}

bool FileServer::broadcast(const Message &msg) {
  std::lock_guard<std::mutex> lock(peerLock_);
  for (const auto &[_, peer] : peers_) {
    // Serialize message
    std::stringstream ss;
    nlohmann::json j = {{"type", static_cast<int>(msg.type)},
                        {"payload", msg.payload}};
    ss << j.dump();

    // Send as regular message
    peer->Send({network::MessageType::IncomingMessage});

    std::string str = ss.str();
    std::vector<uint8_t> data(str.begin(), str.end());
    if (!peer->Send(data)) {
      return false;
    }
  }
  return true;
}

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

void FileServer::messageLoop() {
  while (running_) {
    auto rpcChan = opts_.transport->Consume();
    auto rpc = rpcChan->Receive();

    // Decode message
    try {
      auto j = nlohmann::json::parse(rpc.payload);
      Message msg{.type = static_cast<Message::Type>(j["type"].get<int>()),
                  .payload = j["payload"]};
      handleMessage(rpc.from, msg);
    } catch (const std::exception &e) {
      std::cerr << "Error decoding message: " << e.what() << std::endl;
    }
  }
}

} // namespace server
} // namespace dfs