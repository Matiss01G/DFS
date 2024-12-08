#include "network/peer.h"
#include <gtest/gtest.h>
#include <queue>

namespace dfs {
namespace network {
namespace test {

// Mock implementation of Peer interface for testing purposes
// Simulates network behavior without actual network calls
class MockPeer : public Peer {
public:
  // Constructor stores peer identification info for testing
  MockPeer(std::string id, std::string addr)
      : id_(std::move(id)), addr_(std::move(addr)) {}

  // Simulates sending data by storing it in a queue instead of network
  // transmission
  bool Send(const std::vector<uint8_t> &data) override {
    sent_data_.push(data);
    return true; // Always succeed in test environment
  }

  // Simulates closing a stream by setting a flag
  void CloseStream() override { stream_closed_ = true; }

  // Returns the stored address instead of actual network address
  std::string RemoteAddr() const override { return addr_; }

  // Simulates writing stream data by storing buffer contents
  bool WriteStream(boost::asio::streambuf &buffer) override {
    // Convert streambuf data to vector for easy storage/verification
    std::string data{boost::asio::buffers_begin(buffer.data()),
                     boost::asio::buffers_end(buffer.data())};
    written_stream_data_.push_back(
        std::vector<uint8_t>(data.begin(), data.end()));
    return true;
  }

  // Simulates reading from a stream using pre-set test data
  bool ReadStream(std::ostream &out, size_t bytes) override {
    if (test_data_.empty())
      return false;
    out.write(reinterpret_cast<const char *>(test_data_.data()),
              std::min(bytes, test_data_.size()));
    return true;
  }

  // Test helper methods to verify behavior:

  // Check if CloseStream was called
  bool wasStreamClosed() const { return stream_closed_; }

  // Check if any data was sent
  bool hasSentData() const { return !sent_data_.empty(); }

  // Get the next chunk of data that was "sent"
  std::vector<uint8_t> getLastSentData() {
    if (sent_data_.empty())
      return {};
    auto data = sent_data_.front();
    sent_data_.pop();
    return data;
  }

  // Set data to be returned by ReadStream
  void setTestData(std::vector<uint8_t> data) { test_data_ = std::move(data); }

  // Get all data that was written via WriteStream
  const std::vector<std::vector<uint8_t>> &getWrittenStreamData() const {
    return written_stream_data_;
  }

private:
  std::string id_;                             // Stored peer identifier
  std::string addr_;                           // Stored peer address
  bool stream_closed_ = false;                 // Track if stream was closed
  std::queue<std::vector<uint8_t>> sent_data_; // Queue of "sent" data
  std::vector<uint8_t> test_data_;             // Data to return in ReadStream
  std::vector<std::vector<uint8_t>>
      written_stream_data_; // Data written to streams
};

} // namespace test
} // namespace network
} // namespace dfs