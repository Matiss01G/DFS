/**
 * TCP implementation of the Transport interface.
 *
 * Manages TCP-based communication between nodes in the distributed file system.
 * Handles:
 * - Connection establishment and acceptance
 * - Peer management
 * - Message distribution
 * - Network event loop
 */

#pragma once

#include "network/decoder.h"
#include "network/tcp_peer.h"
#include "network/transport.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace dfs {
namespace network {

// function for performing handshake with new peers
using HandshakeFunc = std::function<bool(TCPPeer &)>;

// empty handshake that always succeedsHand
inline bool NOPHandshakeFunc(TCPPeer &peer) { return true; }

// configuration options for TCP transport
struct TCPTransportOpts {
  std::string listenAddr;                         // address to listen on
  HandshakeFunc handshakeFunc = NOPHandshakeFunc; // handles peer handshakes
  std::function<void(std::shared_ptr<TCPPeer>)> OnPeer; // called when new peer connects
  std::shared_ptr<Decoder> decoder;
};

class TCPTransport : public Transport {
public:
  // creates a new TCP transport with the given options
  // Sets up the listening socket and message channels
  explicit TCPTransport(TCPTransportOpts opts);
  ~TCPTransport();

  // returns address this transport is listening on
  std::string Addr() const override;
  // establishes outbound connection with other node
  bool Dial(const std::string &addr) override;
  // starts accepting inbound connections
  bool ListenAndAccept() override;
  // starts channel that will receive all incoming messages
  std::shared_ptr<Channel<RPC>> Consume() override;

  // shut down the transport
  bool Close() override;

  void setOnPeer(std::function<void(std::shared_ptr<Peer>)> callback) override;

private:
  // private methods for connection management
  // void addPeer(const std::string& addr, std::shared_ptr<TCPPeer> peer); //
  // not in use
  void removePeer(const std::string &addr);
  // std::shared_ptr<TCPPeer> getPeer(const std::string& addr); // not in use
  // void closeAllPeers(); // not in use

  // helper methods for connection handling
  // void handleReadError(const std::string& peerAddr, const std::string&
  // error); // not in use

  // configuration options passed to constructor
  TCPTransportOpts opts_;

  // networking components:
  boost::asio::io_context io_context_;      // event loop for async operations
  boost::asio::ip::tcp::acceptor acceptor_; // accepts incoming tcp connections
  std::shared_ptr<Channel<RPC>> rpcChan_;   // channel for incoming messages

  // atomic flag for clean shutdown
  // std::atomic<bool> running_; // not in use

  // peer management:
  std::mutex peersMutex_; // protects peers_map
  std::unordered_map<std::string, std::shared_ptr<TCPPeer>>
      peers_; // active peers

  // internal helper methods:
  // runs loop that handles incoming connections
  void startAcceptLoop();
  // sets up a new peer connection
  void handleConnection(boost::asio::ip::tcp::socket socket, bool outbound);
};

} // namespace network
} // namespace dfs