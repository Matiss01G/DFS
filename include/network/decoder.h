#pragma once

/**
 * @file decoder.h
 * @brief Message decoding interfaces and implementations for the DFS network layer
 */

#include "network/message.h"
#include <memory>
#include <boost/asio.hpp>

namespace dfs {
namespace network {
  
// Interface for message decoders
class Decoder {
public:
  virtual ~Decoder() = default;

  // Decode a message from a stream
  // Returns number of bytes read if successful, -1 on error
  virtual ssize_t Decode(boost::asio::ip::tcp::socket& socket, RPC& msg) = 0;
};

// Binary protocol decoder implementing the wire format:
// [1 byte]  - Message type (IncomingMessage or IncomingStream)
// [4 bytes] - Payload size (uint32_t, little endian)
// [N bytes] - Payload data
class BinaryDecoder : public Decoder {
public:
  ssize_t Decode(boost::asio::ip::tcp::socket& socket, RPC& msg) override;

private:
  // Helper to read exact number of bytes
  bool ReadExact(boost::asio::ip::tcp::socket& socket, 
                uint8_t* buffer, 
                size_t len);
};

} // namespace network
} // namespace dfs