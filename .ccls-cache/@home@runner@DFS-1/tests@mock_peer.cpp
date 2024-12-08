// mock_peer.cpp
#include "mock_peer.h"

namespace dfs {
namespace network {
namespace test {

bool MockPeer::Send(const std::vector<uint8_t> &data) {
  // Create a new copy of the data and store it
  sent_data_.push(std::vector<uint8_t>(data)); // Properly copies the vector
  return true;
}

void MockPeer::CloseStream() { stream_closed_ = true; }

std::string MockPeer::RemoteAddr() const { return addr_; }

bool MockPeer::WriteStream(boost::asio::streambuf &buffer) {
  // Safely copy data from streambuf
  std::vector<uint8_t> data;
  const char *begin = boost::asio::buffer_cast<const char *>(buffer.data());
  const char *end = begin + boost::asio::buffer_size(buffer.data());
  data.assign(reinterpret_cast<const uint8_t *>(begin),
              reinterpret_cast<const uint8_t *>(end));
  written_stream_data_.push_back(std::move(data));
  return true;
}

bool MockPeer::ReadStream(std::ostream &out, size_t bytes) {
  if (test_data_.empty()) {
    return false;
  }

  size_t to_write = std::min(bytes, test_data_.size());
  out.write(reinterpret_cast<const char *>(test_data_.data()), to_write);
  return out.good();
}

} // namespace test
} // namespace network
} // namespace dfs