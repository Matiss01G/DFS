#include "network/tcp_transport.h"
#include <iostream>

namespace dfs {
namespace network {

TCPTransport::TCPTransport(TCPTransportOpts opts)
    : opts_(std::move(opts)), acceptor_(io_context_),
      rpcChan_(std::make_shared<Channel<RPC>>(1024)) {

  // Parse address and port
  std::string addr = "127.0.0.1"; // default to localhost
  std::string portStr;

  // split address into ip and port.
  // SHOULD BE DEFINED IN HELPER BECAUSE DIAL() ALREADY DOES THIS
  size_t colonPos = opts_.listenAddr.find(':');
  if (colonPos != std::string::npos) {
    if (colonPos > 0) { // if there's an address specified
      addr = opts_.listenAddr.substr(0, colonPos);
    }
    portStr = opts_.listenAddr.substr(colonPos + 1);
  }

  // Create endpoint with proper address binding
  boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address::from_string(addr), std::stoi(portStr));

  // open acceptor
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
}

void dfs::network::TCPTransport::setOnPeer(
    std::function<void(std::shared_ptr<Peer>)> callback) {
  opts_.OnPeer = std::move(callback);
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
    // std::cout << "TCP transport listening on " << opts_.listenAddr <<
    // std::endl;

    // launch accept loop in separate thread
    std::thread([this]() { startAcceptLoop(); }).detach();
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
    // debug print statement
    std::cout << "[Transport " << opts_.listenAddr << "] New "
              << (outbound ? "outbound" : "inbound") << " connection"
              << std::endl;

    // create new peer for this connection
    auto peer =
        std::make_shared<dfs::network::TCPPeer>(std::move(socket), outbound);
    // debug print statement
    std::cout << "[Transport " << opts_.listenAddr << "] Created peer "
              << peer->RemoteAddr() << std::endl;

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
    if (opts_.OnPeer) {
      // debug print statement
      std::cout << "[Transport " << opts_.listenAddr << "] Calling OnPeer for "
                << peer->RemoteAddr() << std::endl;
      opts_.OnPeer(peer);
    }

    // start reading messages from this peer
    // each peer gets its own read loop running in a separate thread
    // Start a read loop for this peer in a separate thread
    std::thread([this, peer]() {
      // debug print statement
      std::cout << "[Transport " << opts_.listenAddr
                << "] Starting read loop for " << peer->RemoteAddr()
                << std::endl;
      while (true) {
        RPC rpc;
        // Use decoder to read next message
        ssize_t bytesRead = opts_.decoder->Decode(peer->socket(), rpc);
        if (bytesRead < 0) {
          std::cerr << "Decode error from peer: " << peer->RemoteAddr()
                    << std::endl;
          break;
        }

        if (rpc.isStream()) {
          peer->StartStream();
          std::cout << "[" << peer->RemoteAddr()
                    << "] incoming stream, waiting..." << std::endl;
          peer->WaitForStream();
          std::cout << "[" << peer->RemoteAddr()
                    << "] stream closed, resuming read loop" << std::endl;
          continue;
        }

        rpc.setFrom(peer->RemoteAddr());
        rpcChan_->Send(std::move(rpc));
      }

      // Clean up peer when loop exits
      removePeer(peer->RemoteAddr());
    }).detach(); // Launch the read loop in a detached thread
  } catch (const std::exception &e) {
    std::cerr << "Handle connection error: " << e.what() << std::endl;
  }
}

// returns the channel where all incoming messages can be received
std::shared_ptr<Channel<RPC>> TCPTransport::Consume() { return rpcChan_; }

// shuts down the transport, closing all connections
bool TCPTransport::Close() {
  // debug print statement
  std::cout << "[Transport " << opts_.listenAddr << "] Close called"
            << std::endl;

  try {
    // stop accepting new connections
    acceptor_.close();
    // debug print statement
    std::cout << "[Transport " << opts_.listenAddr << "] Acceptor closed"
              << std::endl;

    // close all peer connections
    std::lock_guard<std::mutex> lock(peersMutex_);

    // debug print statements
    std::cout << "[Transport " << opts_.listenAddr << "] Closing "
              << peers_.size() << " peers" << std::endl;
    for (auto &[addr, peer] : peers_) {
      std::cout << "[Transport " << opts_.listenAddr << "] Closing peer "
                << addr << std::endl;
    }

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