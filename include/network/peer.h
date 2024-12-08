#pragma once

/**
 * Defines the interface for network peers in the distributed file system.
 *
 * A peer represents a remote node in the network. This interface establishes
 * the contract that all peer implementations must follow, providing basic
 * operations for:
 * - Sending data to remote nodes
 * - Managing streaming operations
 * - Getting remote node addresses
 *
 * This abstraction allows for different types of network connections (TCP, UDP,
 * etc.) to be used interchangeably in the system.
 */

#include "network/peer.h"
#include "message.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>

namespace dfs {
namespace network {

class Peer {
public:
  virtual ~Peer() = default;

  // send raw data to the peer
  virtual bool Send(const std::vector<uint8_t> &data) = 0;

  // called when a stream operation is complete
  virtual void CloseStream() = 0;

  // get the address of the connected peer
  virtual std::string RemoteAddr() const = 0;

  // Sends a single message type byte over the network
  virtual bool SendMessageType(MessageType type) {
    return Send(std::vector<uint8_t>{static_cast<uint8_t>(type)});
  }

  // Writes/sends raw data from a stream buffer to the network
  virtual bool WriteStream(boost::asio::streambuf& buffer) = 0;

  // Reads specified number of bytes from network into the provided output stream
  virtual bool ReadStream(std::ostream& out, size_t bytes) = 0;
};

} // namespace network
} // namespace dfs