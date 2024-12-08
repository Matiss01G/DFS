#pragma once
#include "network/channel.h"
#include "network/transport.h"
#include <atomic>
#include <memory>

namespace dfs {
namespace network {
namespace test {

class MockTransport : public Transport {
public:
  explicit MockTransport(std::string addr = ":3000")
      : addr_(std::move(addr)), rpcChan_(std::make_shared<Channel<RPC>>(1024)),
        running_(false) {
    // Ensure channel is properly initialized
    if (!rpcChan_) {
      throw std::runtime_error("Failed to initialize RPC channel");
    }
  }

  std::string Addr() const override { return addr_; }

  bool Dial(const std::string &addr) override {
    if (!running_) {
      return false;
    }
    last_dialed_ = addr;
    return true;
  }

  bool ListenAndAccept() override {
    if (!rpcChan_) {
      return false;
    }
    running_ = true;
    return true;
  }

  std::shared_ptr<Channel<RPC>> Consume() override {
    if (!running_) {
      return nullptr;
    }
    return rpcChan_;
  }

  bool Close() override {
    running_ = false;
    // Clear channel when closing
    while (!rpcChan_->Empty()) {
      rpcChan_->TryReceive();
    }
    return true;
  }

  void setOnPeer(std::function<void(Peer &)> callback) override {
    onPeer_ = std::move(callback);
  }

  // Test helper methods
  const std::string &getLastDialed() const { return last_dialed_; }
  bool isRunning() const { return running_; }
  void simulateMessage(RPC msg) {
    if (running_) {
      rpcChan_->TrySend(std::move(msg));
    }
  }

private:
  std::string addr_;
  std::shared_ptr<Channel<RPC>> rpcChan_;
  std::string last_dialed_;
  std::atomic<bool> running_;
  std::function<void(Peer &)> onPeer_;
};

} // namespace test
} // namespace network
} // namespace dfs