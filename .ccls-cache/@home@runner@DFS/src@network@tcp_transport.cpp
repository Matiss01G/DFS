#include "network/tcp_transport.h"
#include <iostream>

namespace dfs {
namespace network {

TCPTransport::TCPTransport(TCPTransportOpts opts)
    : opts_(std::move(opts)), acceptor_(io_context_),
      rpcChan_(std::make_shared<Channel<RPC>>(1024)) {

  // parse listening address into TCP endpoint
  boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::tcp::v4(), std::stoi(opts_.listenAddr.substr(1)));

  // open acceptor
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
}

// destructor ensures all connections are properly closed
TCPTransport::~TCPTransport() { Close(); }

std::string dfs::network::TCPTransport::Addr() const {
    return opts_.listenAddr;
}

bool TCPTransport::Dial(const std::string &addr) {
  try {
    // create a new socket for this connection
    boost::asio::ip::tcp::socket socket(io_context_);

    // split address into host and port components
    size_t colon_pos = addr.find(':');
    if (colon_pos == std::string::npos) {
      return false;
    }

    std::string host = addr.substr(0, colon_pos);
    int port = std::stoi(addr.substr(colon_pos + 1));

    // resolve the address to one or more endpoints
    boost::asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    // attempt to connect to the resolved endpoints
    boost::asio::connect(socket, endpoints);

    // handle the new outbound connection
    handleConnection(std::move(socket), true);
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Dial error: " << e.what() << std::endl;
    return false;
  }
}

// listen for incoming connections and start accept loop
bool TCPTransport::ListenAndAccept() {
  try {
    // start listening
    acceptor_.listen();
    // std::cout << "TCP transport listening on " << opts_.listenAddr << std::endl;

    // launch accept loop in separate thread
    std::thread([this]() {
        startAcceptLoop();
    }).detach();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Listen error: " << e.what() << std::endl;
    return false;
  }
}

// main loop that accepts incoming connections
void TCPTransport::startAcceptLoop() {
  while (acceptor_.is_open()) {
    try {
      // wait for and accept new connections
      boost::asio::ip::tcp::socket socket(io_context_);
      acceptor_.accept(socket);

      // handle inbound connection
      handleConnection(std::move(socket), false);
    } catch (const std::exception &e) {
      if (!acceptor_.is_open()) {
        break;
      }
      std::cerr << "Accept error: " << e.what() << std::endl;
    }
  }
}

// sets up and manages new peer connection whether inbound/outbound
void TCPTransport::handleConnection(boost::asio::ip::tcp::socket socket,
                                    bool outbound) {
  try {
    // create new peer for this connection
    auto peer = std::make_shared<dfs::network::TCPPeer>(std::move(socket), outbound);
    // perform the handshake protocol
    if (!opts_.handshakeFunc(*peer)) {
      std::cerr << "Handshake failed with " << peer->RemoteAddr() << std::endl;
      return;
    }

    // add peer to our active peers map
    {
      std::lock_guard<std::mutex> lock(peersMutex_);
      peers_[peer->RemoteAddr()] = peer;
    }

    // notify callback if one was provided
    if (opts_.onPeer) {
      opts_.onPeer(*peer);
    }

    // std::cout << "Connected with peer: " << peer->RemoteAddr() << std::endl;

    // start reading messages from this peer
    // each peer gets its own read loop running in a separate thread
    // Start a read loop for this peer in a separate thread
    std::thread([this, peer]() {
      while (true) {
        RPC rpc;
        // Use decoder to read next message
        ssize_t bytesRead = opts_.decoder->Decode(peer->socket(), rpc);
        if (bytesRead < 0) {
          std::cerr << "Decode error from peer: " << peer->RemoteAddr() << std::endl;
          break;
        }

        if (rpc.isStream()) {
          peer->StartStream();
          std::cout << "[" << peer->RemoteAddr() << "] incoming stream, waiting..." 
                   << std::endl;
          peer->WaitForStream();
          std::cout << "[" << peer->RemoteAddr() << "] stream closed, resuming read loop" 
                   << std::endl;
          continue;
      }

        rpc.setFrom(peer->RemoteAddr());
        rpcChan_->Send(std::move(rpc));
      }

      // Clean up peer when loop exits
      removePeer(peer->RemoteAddr());

    }).detach();  // Launch the read loop in a detached thread
  } catch (const std::exception &e) {
    std::cerr << "Handle connection error: " << e.what() << std::endl;
  }
}

// returns the channel where all incoming messages can be received
std::shared_ptr<Channel<RPC>> TCPTransport::Consume() { 
  return rpcChan_; 
}

// shuts down the transport, closing all connections
bool TCPTransport::Close() {
  try {
    // stop accepting new connections
    acceptor_.close();

    // close all peer connections
    std::lock_guard<std::mutex> lock(peersMutex_);
    peers_.clear();

    return true;
  } catch (const std::exception &e) {
    std::cerr << "Close error: " << e.what() << std::endl;
    return false;
  }
}

// Removes a peer from active peers map
// called when a peer disconnects
void TCPTransport::removePeer(const std::string &addr) {
  std::lock_guard<std::mutex> lock(peersMutex_);
  peers_.erase(addr);
}

} // namespace network
} // namespace dfs