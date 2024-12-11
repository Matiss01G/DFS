#include "network/decoder.h"
#include "logging/logging.hpp"
#include <chrono>
#include <thread>

namespace dfs {
namespace network {

ssize_t BinaryDecoder::Decode(boost::asio::ip::tcp::socket &socket, RPC &msg) {
  try {
    socket.non_blocking(true);

    // Read message type byte
    uint8_t msgType;
    if (!ReadExact(socket, &msgType, 1)) {
      // Check if socket is still connected
      boost::system::error_code ec;
      if (!socket.is_open()) {
        LOG_INFO << "Connection closed by peer";
        return -1;
      }
      // Socket is still open but no data yet
      return 0;
    }

    // Validate message type
    if (msgType != static_cast<uint8_t>(MessageType::IncomingMessage) &&
        msgType != static_cast<uint8_t>(MessageType::IncomingStream)) {
      LOG_ERROR << "Invalid message type: " << static_cast<int>(msgType);
      return -1;
    }

    // Handle stream message
    if (msgType == static_cast<uint8_t>(MessageType::IncomingStream)) {
      msg.setStream(true);
      return 1;
    }

    // Read payload size
    uint32_t networkPayloadSize;
    if (!ReadExact(socket, reinterpret_cast<uint8_t *>(&networkPayloadSize),
                   sizeof(networkPayloadSize))) {
      return 0; // Try again later
    }

    uint32_t payloadSize = ntohl(networkPayloadSize);
    if (payloadSize > 1024 * 1024) { // 1MB limit
      LOG_ERROR << "Payload size too large: " << payloadSize;
      return -1;
    }

    // Read payload
    std::vector<uint8_t> payload(payloadSize);
    if (!ReadExact(socket, payload.data(), payloadSize)) {
      return 0; // Try again later
    }

    msg.setStream(false);
    msg.setPayload(std::move(payload));
    return 1 + sizeof(payloadSize) + payloadSize;

  } catch (const boost::system::system_error &e) {
    if (e.code() == boost::asio::error::eof ||
        e.code() == boost::asio::error::connection_reset ||
        e.code() == boost::asio::error::connection_aborted) {
      LOG_INFO << "Connection terminated: " << e.what();
      return -1;
    }
    LOG_ERROR << "Socket error: " << e.what();
    return 0; // Any other error - let caller retry
  }
}

bool BinaryDecoder::ReadExact(boost::asio::ip::tcp::socket &socket,
                              uint8_t *buffer, size_t len) {
  size_t totalRead = 0;
  boost::system::error_code ec;

  while (totalRead < len) {
    size_t bytesRead = socket.read_some(
        boost::asio::buffer(buffer + totalRead, len - totalRead), ec);

    if (ec == boost::asio::error::would_block) {
      // No data available right now - that's normal
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    if (ec == boost::asio::error::eof ||
        ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::connection_aborted) {
      LOG_INFO << "Connection terminated during read";
      return false;
    }

    if (ec) {
      LOG_ERROR << "Read error: " << ec.message();
      return false;
    }

    if (bytesRead == 0) {
      if (!socket.is_open()) {
        LOG_INFO << "Socket closed during read";
        return false;
      }
      // Socket still open but no data - keep trying
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    totalRead += bytesRead;
  }

  return true;
}

} // namespace network
} // namespace dfs