#pragma once

/**
  TCP-based implementation of the Peer interface for the distributed file
  system.

  TCPPeer manages individual TCP connections with other nodes in the network.
  It provides:
  - Connection management for both inbound and outbound connections
  - Thread-safe data transmission
  - Stream operation handling

  Each TCPPeer instance represents a single TCP connection to another node,
  handling all communication with that specific peer. The class tracks whether
  the connection was initiated locally (outbound) or remotely (inbound) and
  manages the lifecycle of the TCP socket.
*/

#include "network/peer.h"
#include <boost/asio.hpp>
#include <memory>
#include <mutex>

namespace dfs {
namespace network {

class TCPPeer : public Peer {
public:
  // creates a TCP peer from an existing connection
  TCPPeer(boost::asio::ip::tcp::socket socket, bool outbound);

  // Implement peer interface
  bool Send(const std::vector<uint8_t>& data) override;
  void CloseStream() override;
  std::string RemoteAddr() const override;

  void StartStream();
  void WaitForStream();

  boost::asio::ip::tcp::socket& socket() { return socket_; }
  
private:
  boost::asio::ip::tcp::socket socket_;
  bool outbound_;               // true if we initiate connection

  // stream synchronization 
  std::condition_variable stream_cv_;
  std::mutex stream_mutex_;     // proctects stream operations
  bool stream_complete_ = true; // true when no stream in progress
};

} // namespace network
} // namespace dfs