#include "network/tcp_peer.h"
#include "logging/logging.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <thread>

namespace dfs {
namespace network {

TCPPeer::TCPPeer(boost::asio::ip::tcp::socket socket, bool outbound)
    : socket_(std::move(socket)), outbound_(outbound) {
  LOG_DEBUG << "[" << RemoteAddr() << "] New "
            << (outbound_ ? "outbound" : "inbound") << " peer created";
}

bool TCPPeer::Send(const std::vector<uint8_t> &data) {
  try {
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(data), ec);
    if (ec) {
      LOG_ERROR << "[" << RemoteAddr()
                << "] Failed to send data: " << ec.message();
      return false;
    }
    LOG_TRACE << "[" << RemoteAddr() << "] Sent " << data.size() << " bytes";
    return true;
  } catch (const boost::system::system_error &e) {
    LOG_ERROR << "[" << RemoteAddr() << "] Send error: " << e.what();
    return false;
  }
}

void TCPPeer::CloseStream() {
  {
    std::lock_guard<std::mutex> lock(stream_mutex_);
    stream_complete_ = true;
    LOG_DEBUG << "[" << RemoteAddr() << "] Stream marked as complete";
  }
  stream_cv_.notify_all();
}

void TCPPeer::StartStream() {
  std::lock_guard<std::mutex> lock(stream_mutex_);
  stream_complete_ = false;
  LOG_DEBUG << "[" << RemoteAddr() << "] Starting new stream";
}

void TCPPeer::WaitForStream() {
  std::unique_lock<std::mutex> lock(stream_mutex_);
  LOG_DEBUG << "[" << RemoteAddr() << "] Waiting for stream completion";
  stream_cv_.wait(lock, [this]() { return stream_complete_; });
  LOG_DEBUG << "[" << RemoteAddr() << "] Stream wait completed";
}

std::string TCPPeer::RemoteAddr() const {
  try {
    return socket_.remote_endpoint().address().to_string() + ":" +
           std::to_string(socket_.remote_endpoint().port());
  } catch (const boost::system::system_error &e) {
    LOG_ERROR << "Failed to get remote address: " << e.what();
    return "";
  }
}

bool TCPPeer::WriteStream(boost::asio::streambuf &buffer) {
  try {
    size_t bytes = boost::asio::write(socket_, buffer);
    LOG_DEBUG << "[" << RemoteAddr() << "] Wrote " << bytes
              << " bytes to stream";
    return true;
  } catch (const boost::system::system_error &e) {
    LOG_ERROR << "[" << RemoteAddr() << "] Stream write error: " << e.what();
    return false;
  }
}

bool TCPPeer::ReadStream(std::ostream &out, size_t bytes) {
  try {
    LOG_DEBUG << "Starting read stream, expecting " << bytes << " bytes";
    std::vector<char> temp(bytes);
    size_t total_read = 0;
    const int MAX_RETRIES = 50;
    int retry_count = 0;

    while (total_read < bytes) {
      LOG_DEBUG << "Current total_read: " << total_read << " out of " << bytes
                << " bytes";
      boost::system::error_code ec;
      size_t n = boost::asio::read(
          socket_,
          boost::asio::buffer(temp.data() + total_read, bytes - total_read),
          ec);

      if (ec == boost::asio::error::would_block) {
        retry_count++;
        if (retry_count >= MAX_RETRIES) {
          LOG_ERROR << "[" << RemoteAddr() << "] Max retries (" << MAX_RETRIES
                    << ") exceeded while waiting for data";
          return false;
        }
        // No data available right now, wait a bit and retry
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }

      // Reset retry count on successful read
      retry_count = 0;

      if (ec) {
        // Real error occurred
        stream_complete_ = true;
        LOG_ERROR << "[" << RemoteAddr()
                  << "] Stream read error: " << ec.message();
        return false;
      }

      if (n == 0) {
        if (!socket_.is_open()) {
          LOG_ERROR << "[" << RemoteAddr() << "] Socket closed during read";
          return false;
        }
        // Socket still open but no data - keep trying
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }

      total_read += n;
    }

    // Write all data at once after complete read
    out.write(temp.data(), bytes);
    CloseStream();
    LOG_DEBUG << "[" << RemoteAddr() << "] Read " << bytes
              << " bytes from stream";
    return true;

  } catch (const std::exception &e) {
    stream_complete_ = true;
    LOG_ERROR << "[" << RemoteAddr()
              << "] Unexpected stream error: " << e.what();
    return false;
  }
}

} // namespace network
} // namespace dfs