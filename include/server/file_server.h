#pragma once

/**
brief File server implementation for distributed file system

Provides file storage and retrieval across a distributed network of nodes.
Handles peer discovery, file replication, and encrypted transfers.
 */

#include "crypto/crypto_utils.h"
#include "network/peer.h"
#include "network/transport.h"
#include "storage/path_key.h"
#include "storage/store.h"
#include <memory>
#include <mutex>
#include <string>
#include <third_party/json.hpp>
#include <thread>
#include <unordered_map>

namespace dfs {
namespace server {

class FileServer;

struct FileServerOpts {
  std::string id;              // Unique server identifier
  std::vector<uint8_t> encKey; // Encryption key for file transfer
  std::string storageRoot;     // Root directory for file storage
  storage::PathTransformFunc pathTransformFunc;  // Path transformation strategy
  std::shared_ptr<network::Transport> transport; // Network transport layer
  std::vector<std::string> bootstrapNodes;       // Initial peer nodes
};

// Message types for inter-node communication
struct Message {
  enum class Type { StoreFile, GetFile };
  Type type{}; // Added default member initializer
  nlohmann::json payload;
};

class FileServer {
public:
  explicit FileServer(FileServerOpts opts);
  ~FileServer();

  // Prevent copying
  FileServer(const FileServer &) = delete;
  FileServer &operator=(const FileServer &) = delete;

  // Core operations
  bool Start();
  void Stop();

  // File operations
  bool Store(const std::string &key, std::istream &data);
  std::unique_ptr<std::istream> Get(const std::string &key);

  // Network callbacks
  bool OnPeer(std::shared_ptr<network::Peer> peer);

private:
  // Message handlers
  void handleMessage(const std::string &from, const Message &msg);
  bool handleStoreFile(const std::string &from, const nlohmann::json &payload);
  bool handleGetFile(const std::string &from, const nlohmann::json &payload);

  // Network operations
  bool broadcast(const Message &msg);
  void bootstrapNetwork();
  void messageLoop();

  // Member variables
  FileServerOpts opts_;
  std::unique_ptr<storage::Store> store_;

  // Peer management
  std::mutex peerLock_;
  std::unordered_map<std::string, std::shared_ptr<network::Peer>> peers_;

  // Control
  std::atomic<bool> running_{false};
  std::unique_ptr<std::thread> loopThread_;
};

} // namespace server
} // namespace dfs