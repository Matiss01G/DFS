/**
 * Transport interface for the distributed file system.
 *
 * This interface defines how nodes communicate in the network.
 * It abstracts the underlying transport mechanism (TCP, UDP, etc)
 * and provides methods for:
 * - Establishing connections (Dial)
 * - Accepting incoming connections (ListenAndAccept)
 * - Receiving messages (Consume)
 * - Getting local address info (Addr)
 */

#pragma once

#include "network/channel.h"
#include "network/message.h"
#include <memory>
#include <string>

namespace dfs {
namespace network {

class Transport {
public:
  virtual ~Transport() = default;

  // Returns the address this transport is listening on
  virtual std::string Addr() const = 0;

  // Establishes a connection with a remote node
  virtual bool Dial(const std::string &addr) = 0;

  // Starts listening for and accepting incoming connections
  virtual bool ListenAndAccept() = 0;

  // Returns a channel of incoming RPCs from the network
  virtual std::shared_ptr<Channel<RPC>> Consume() = 0;

  // Closes the transport and all active connections
  virtual bool Close() = 0;
};

} // namespace network
} // namespace dfs