#pragma once
/**
 * @file mock_peer.h
 * @brief Mock implementation of the Peer interface for testing
 *
 * Provides a mock peer implementation that can be used in unit tests to verify
 * network communication behavior without requiring actual network connections.
 * Tracks all sent/received data and provides helper methods for test verification.
 */
#include "network/peer.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>
#include <queue>
#include <algorithm>

namespace dfs {
namespace network {
namespace test {

class MockPeer : public Peer {
public:
  // Constructor takes an ID and address for the mock peer
  MockPeer(std::string id, std::string addr)
    : id_(std::move(id))
    , addr_(std::move(addr)) 
  {}

  // Implementation of Peer interface
  bool Send(const std::vector<uint8_t>& data) override {
    // Create a new copy of the data and store it
    sent_data_.push(std::vector<uint8_t>(data));
    return true;
  }

  void CloseStream() override { 
    stream_closed_ = true; 
  }

  std::string RemoteAddr() const override { 
    return addr_; 
  }

  bool WriteStream(boost::asio::streambuf& buffer) override {
    // Safely copy data from streambuf
    std::vector<uint8_t> data;
    const char* begin = boost::asio::buffer_cast<const char*>(buffer.data());
    const char* end = begin + boost::asio::buffer_size(buffer.data());

    data.assign(reinterpret_cast<const uint8_t*>(begin),
           reinterpret_cast<const uint8_t*>(end));

    written_stream_data_.push_back(std::move(data));
    return true;
  }

  bool ReadStream(std::ostream& out, size_t bytes) override {
    if (test_data_.empty()) {
      return false;
    }

    size_t to_write = std::min(bytes, test_data_.size());
    out.write(reinterpret_cast<const char*>(test_data_.data()), to_write);

    return out.good();
  }

  // Test helper methods
  bool wasStreamClosed() const { return stream_closed_; }
  bool hasSentData() const { return !sent_data_.empty(); }

  std::vector<uint8_t> getLastSentData() {
    if (sent_data_.empty()) {
      return {};
    }
    auto data = std::move(sent_data_.front());
    sent_data_.pop();
    return data;
  }

  void setTestData(std::vector<uint8_t> data) { 
    test_data_ = std::move(data); 
  }

  const std::vector<std::vector<uint8_t>>& getWrittenStreamData() const {
    return written_stream_data_;
  }

private:
  std::string id_;                                    // Peer identifier
  std::string addr_;                                  // Mock network address
  bool stream_closed_ = false;                        // Track stream state
  std::queue<std::vector<uint8_t>> sent_data_;       // Queue of sent data
  std::vector<uint8_t> test_data_;                   // Data to return in reads
  std::vector<std::vector<uint8_t>> written_stream_data_; // Track stream writes
};

} // namespace test
} // namespace network
} // namespace dfs