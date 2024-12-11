#include "server/file_server.h"
#include "logging/logging.hpp"
#include <iostream>
#include <sstream>
#include <thread>

namespace dfs {
namespace server {

FileServer::FileServer(FileServerOpts opts) : opts_(std::move(opts)) {
  // Get the full address from transport or use default
  std::string node_addr =
      opts_.transport ? opts_.transport->Addr() : "0.0.0.0:0";

  // Initialize logging with node address
  // logging::init_logging(node_addr);

  LOG_INFO << "Initializing FileServer...";

  if (opts_.id.empty()) {
    opts_.id = crypto::generateId();
  }

  storage::StoreOpts storeOpts{.root = opts_.storageRoot,
                               .pathTransformFunc = opts_.pathTransformFunc};
  store_ = std::make_unique<storage::Store>(storeOpts);

  if (opts_.transport) {
    opts_.transport->setOnPeer([this](std::shared_ptr<network::Peer> peer) {
      return this->OnPeer(peer);
    });
  }
}

FileServer::~FileServer() { Stop(); }

bool FileServer::Start() {
  if (running_) {
    LOG_WARN << "Attempted to start already running FileServer";
    return false;
  }

  LOG_INFO << "[" << opts_.transport->Addr() << "] Starting fileserver...";

  if (!opts_.transport->ListenAndAccept()) {
    LOG_ERROR << "Failed to start transport on address: "
              << opts_.transport->Addr();
    return false;
  }

  auto rpcChan = opts_.transport->Consume();
  if (!rpcChan) {
    LOG_ERROR << "Failed to initialize RPC channel";
    return false;
  }

  running_ = true;
  loopThread_ = std::make_unique<std::thread>([this]() {
    try {
      this->messageLoop();
    } catch (const std::exception &e) {
      LOG_ERROR << "Message loop error: " << e.what();
      running_ = false;
    }
  });

  bootstrapNetwork();
  return true;
}

void FileServer::Stop() {
  if (!running_)
    return;

  LOG_INFO << "[" << opts_.transport->Addr() << "] Stopping fileserver...";
  running_ = false;

  if (loopThread_ && loopThread_->joinable()) {
    LOG_INFO << "Waiting for loop thread to finish...";
    loopThread_->join();
    LOG_INFO << "Loop thread has finished.";
  }

  if (opts_.transport) {
    opts_.transport->Close();
  }

  LOG_INFO << "[" << opts_.transport->Addr() << "] Server stopped";
}

bool FileServer::Store(const std::string &key, std::istream &data) {
  LOG_INFO << "[" << opts_.transport->Addr()
           << "] Storing file with key: " << key;

  // Store original data in memory buffer
  auto fileBuffer = std::make_shared<std::stringstream>();
  *fileBuffer << data.rdbuf();

  // std::string originalContent = fileBuffer->str();
  // LOG_DEBUG << "Original content: [" << originalContent
  //           << "] Length: " << originalContent.length();

  // // Log key details
  // LOG_DEBUG << "Encryption key size: " << opts_.encKey.size();
  // LOG_DEBUG << "First 4 bytes of key: " << std::hex
  //           << static_cast<int>(opts_.encKey[0]) << " "
  //           << static_cast<int>(opts_.encKey[1]) << " "
  //           << static_cast<int>(opts_.encKey[2]) << " "
  //           << static_cast<int>(opts_.encKey[3]) << std::dec;

  // // Logging original content
  // fileBuffer->seekg(0);
  // LOG_DEBUG << "Original content before encryption: " << originalContent;
  // fileBuffer->seekg(0);

  // Write to local store first
  int64_t size = store_->Write(opts_.id, key, *fileBuffer);
  if (size < 0) {
    LOG_ERROR << "Failed to write file to store";
    return false;
  }

  // Broadcast store message
  Message msg{.type = Message::Type::StoreFile,
              .payload = {{"id", opts_.id}, {"key", key}, {"size", size + 16}}};

  if (!broadcast(msg)) {
    LOG_ERROR << "Failed to broadcast store message";
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // Create a list of active peers with mutex protection
  std::vector<network::Peer *> activePeers;
  {
    std::lock_guard<std::mutex> lock(peerLock_);
    for (const auto &[_, peer] : peers_) {
      activePeers.push_back(peer.get());
    }
  }

  // Set up encryption buffer
  boost::asio::streambuf encryptedBuffer;
  std::ostream encryptedStream(&encryptedBuffer);

  // Reset file buffer to beginning and encrypt so we know final size
  fileBuffer->seekg(0);
  auto encryptedSize =
      crypto::copyEncrypt(opts_.encKey, *fileBuffer, encryptedStream);
  if (encryptedSize < 0) {
    LOG_ERROR << "Failed to encrypt file data";
    return false;
  }

  // Log encrypted size
  LOG_DEBUG << "Size of encrypted data: " << encryptedSize;

  // Convert size to network byte order
  uint32_t networkPayloadSize = htonl(static_cast<uint32_t>(encryptedSize));

  // Send to each peer
  size_t successCount = 0;
  for (auto peer : activePeers) {
    try {
      // Send stream indicator
      if (!peer->SendMessageType({network::MessageType::IncomingStream})) {
        LOG_ERROR << "Failed to send stream indicator to peer";
        continue;
      }

      // Send payload size
      if (!peer->Send(std::vector<uint8_t>(
              reinterpret_cast<uint8_t *>(&networkPayloadSize),
              reinterpret_cast<uint8_t *>(&networkPayloadSize) +
                  sizeof(networkPayloadSize)))) {
        LOG_ERROR << "Failed to send payload size to peer";
        continue;
      }

      encryptedBuffer.pubseekpos(0);

      // Send encrypted data
      if (!peer->WriteStream(encryptedBuffer)) {
        LOG_ERROR << "Failed to send encrypted data to peer";
        continue;
      }

      successCount++;
      peer->CloseStream();
    } catch (const std::exception &e) {
      LOG_ERROR << "Exception while sending to peer: " << e.what();
      continue;
    }
  }

  LOG_INFO << "Successfully sent file to " << successCount << "/"
           << activePeers.size() << " peers";
  return successCount > 0 || activePeers.empty();
}

std::unique_ptr<std::istream> FileServer::Get(const std::string &key) {
  if (store_->Has(opts_.id, key)) {
    auto [size, stream] = store_->Read(opts_.id, key);
    return std::move(stream);
  }

  // LOG_INFO << "[" << opts_.transport->Addr()
  //          << "] Fetching file from network: " << key;

  // Message msg{.type = Message::Type::GetFile,
  //             .payload = {{"id", opts_.id}, {"key", crypto::hashKey(key)}}};

  // if (!broadcast(msg)) {
  //   LOG_ERROR << "Failed to broadcast get request";
  //   return nullptr;
  // }

  // std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  // if (store_->Has(opts_.id, key)) {
  //   auto [size, stream] = store_->Read(opts_.id, key);
  //   return std::move(stream);
  // }

  // LOG_WARN << "File not found in network: " << key;
  LOG_WARN << "File not found locally (network fetch temporarily disabled for "
              "testing purposes): "
           << key;

  return nullptr;
}

bool FileServer::OnPeer(std::shared_ptr<network::Peer> peer) {
  std::lock_guard<std::mutex> lock(peerLock_);
  peers_[peer->RemoteAddr()] = peer;
  LOG_INFO << "Connected with peer: " << peer->RemoteAddr();
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
    default:
      LOG_WARN << "Unknown message type: " << static_cast<int>(msg.type);
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "Error handling message from " << from << ": " << e.what();
  }
}

bool FileServer::handleStoreFile(const std::string &from,
                                 const nlohmann::json &payload) {
  LOG_DEBUG << "Handling store file from: " << from;
  LOG_DEBUG << "Using encryption key size: " << opts_.encKey.size();
  LOG_DEBUG << "First 4 bytes of encryption key: " << std::hex
            << static_cast<int>(opts_.encKey[0]) << " "
            << static_cast<int>(opts_.encKey[1]) << " "
            << static_cast<int>(opts_.encKey[2]) << " "
            << static_cast<int>(opts_.encKey[3]) << std::dec;

  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    LOG_ERROR << "Unknown peer: " << from;
    return false;
  }

  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();
  auto size = payload["size"].get<int64_t>();

  // Read the protocol headers first (5 bytes total)
  std::stringstream headerStream;
  if (!peer->second->ReadStream(headerStream, 5)) {
    LOG_ERROR << "Failed to read message headers";
    return false;
  }

  std::stringstream encryptedData;
  std::stringstream decryptedData;

  try {
    // Read encrypted data (which starts with IV)
    if (!peer->second->ReadStream(encryptedData, size)) {
      LOG_ERROR << "Failed to read encrypted data from peer: " << from;
      return false;
    }

    // Log received encrypted size
    LOG_DEBUG << "Size of received encrypted data: " << encryptedData.tellp();
    LOG_DEBUG << "Read encrypted data size: " << size;
    std::string encryptedStr = encryptedData.str();
    LOG_DEBUG << "First 4 bytes of encrypted data: " << std::hex
              << static_cast<int>(static_cast<uint8_t>(encryptedStr[0])) << " "
              << static_cast<int>(static_cast<uint8_t>(encryptedStr[1])) << " "
              << static_cast<int>(static_cast<uint8_t>(encryptedStr[2])) << " "
              << static_cast<int>(static_cast<uint8_t>(encryptedStr[3]))
              << std::dec;

    // Decrypt data
    auto bytesProcessed =
        crypto::copyDecrypt(opts_.encKey, encryptedData, decryptedData);
    if (bytesProcessed < 0) {
      LOG_ERROR << "Failed to decrypt data from peer: " << from;
      return false;
    }

    LOG_DEBUG << "Decrypted data: [" << decryptedData.str() << "]";
    // Log decrypted size
    LOG_INFO << "Size of decrypted data: " << bytesProcessed;

    // Logging to show decrypted content
    std::string content = decryptedData.str();
    LOG_DEBUG << "Decrypted content for key '" << key << "': " << content;

    // Write to store
    auto result = store_->Write(id, key, decryptedData);
    if (result < 0) {
      LOG_ERROR << "Failed to write data to store";
      return false;
    }

    LOG_INFO << "Successfully stored " << result << " bytes from peer " << from;
    peer->second->CloseStream();
    return true;

  } catch (const std::exception &e) {
    LOG_ERROR << "Exception in handleStoreFile: " << e.what();
    return false;
  }
}

bool FileServer::handleGetFile(const std::string &from,
                               const nlohmann::json &payload) {
  auto id = payload["id"].get<std::string>();
  auto key = payload["key"].get<std::string>();

  if (!store_->Has(id, key)) {
    return false;
  }

  auto peer = peers_.find(from);
  if (peer == peers_.end()) {
    LOG_ERROR << "Unknown peer: " << from;
    return false;
  }

  auto [fileSize, stream] = store_->Read(id, key);
  if (!stream) {
    LOG_ERROR << "Failed to read file from store";
    return false;
  }

  peer->second->SendMessageType({network::MessageType::IncomingStream});
  uint64_t networkFileSize = htobe64(fileSize);
  peer->second->Send(
      std::vector<uint8_t>{reinterpret_cast<const uint8_t *>(&networkFileSize),
                           reinterpret_cast<const uint8_t *>(&networkFileSize) +
                               sizeof(networkFileSize)});

  boost::asio::streambuf encryptedBuffer;
  std::ostream encryptedStream(&encryptedBuffer);

  auto bytesProcessed =
      crypto::copyEncrypt(opts_.encKey, *stream, encryptedStream);
  if (bytesProcessed < 0) {
    LOG_ERROR << "Failed to encrypt file for transfer";
    return false;
  }

  return peer->second->WriteStream(encryptedBuffer);
}

bool FileServer::broadcast(const Message &msg) {
  std::lock_guard<std::mutex> lock(peerLock_);

  if (peers_.empty()) {
    return true; // No peers to broadcast to
  }

  size_t successCount = 0;
  for (const auto &[addr, peer] : peers_) {
    std::stringstream ss;
    nlohmann::json j = {{"type", static_cast<int>(msg.type)},
                        {"payload", msg.payload}};
    ss << j.dump();

    if (!peer->SendMessageType({network::MessageType::IncomingMessage})) {
      continue;
    }

    std::string str = ss.str();
    std::vector<uint8_t> data(str.begin(), str.end());

    uint32_t payloadSize = htonl(static_cast<uint32_t>(data.size()));
    if (!peer->Send(std::vector<uint8_t>(
            reinterpret_cast<uint8_t *>(&payloadSize),
            reinterpret_cast<uint8_t *>(&payloadSize) + sizeof(payloadSize)))) {
      continue;
    }

    if (peer->Send(data)) {
      successCount++;
    }
  }

  if (successCount == 0 && !peers_.empty()) {
    LOG_ERROR << "Failed to broadcast to any peers";
    return false;
  }

  return true;
}

void FileServer::bootstrapNetwork() {
  for (const auto &addr : opts_.bootstrapNodes) {
    if (addr.empty())
      continue;

    std::thread([this, addr]() {
      LOG_INFO << "Connecting to bootstrap node: " << addr;
      if (!opts_.transport->Dial(addr)) {
        LOG_ERROR << "Failed to connect to bootstrap node: " << addr;
      }
    }).detach();
  }
}

void FileServer::messageLoop() {
  auto rpcChan = opts_.transport->Consume();
  if (!rpcChan) {
    LOG_ERROR << "Failed to get message channel from transport";
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
      LOG_ERROR << "Failed to process message: " << e.what();
    }
  }
}

} // namespace server
} // namespace dfs