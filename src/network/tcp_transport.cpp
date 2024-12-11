#include "network/tcp_transport.h"
#include "logging/logging.hpp"
#include <iostream>
#include <thread>

namespace dfs {
namespace network {

TCPTransport::TCPTransport(TCPTransportOpts opts)
    : opts_(std::move(opts)), acceptor_(io_context_),
      rpcChan_(std::make_shared<Channel<RPC>>(1024)) {

  LOG_INFO << "Initializing TCP Transport...";

  // Parse address and port
  std::string addr = "127.0.0.1"; // default to localhost
  std::string portStr;

  // split address into ip and port.
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

  LOG_INFO << "TCP Transport initialized on " << opts_.listenAddr;
}

void TCPTransport::setOnPeer(
    std::function<void(std::shared_ptr<Peer>)> callback) {
  LOG_DEBUG << "Setting OnPeer callback";
  opts_.OnPeer = std::move(callback);
}

TCPTransport::~TCPTransport() {
  LOG_INFO << "Destroying TCP Transport";
  Close();
}

std::string TCPTransport::Addr() const { return opts_.listenAddr; }

bool TCPTransport::Dial(const std::string &addr) {
  LOG_INFO << "Attempting to dial: " << addr;

  try {
    // create a new socket for this connection
    boost::asio::ip::tcp::socket socket(io_context_);

    // split address into host and port components
    size_t colon_pos = addr.find(':');
    if (colon_pos == std::string::npos) {
      LOG_ERROR << "Invalid address format: " << addr;
      return false;
    }

    std::string host = addr.substr(0, colon_pos);
    int port = std::stoi(addr.substr(colon_pos + 1));

    LOG_DEBUG << "Resolving address: " << host << ":" << port;

    // resolve the address to one or more endpoints
    boost::asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    // attempt to connect to the resolved endpoints
    boost::asio::connect(socket, endpoints);
    LOG_INFO << "Successfully connected to: " << addr;

    // handle the new outbound connection
    handleConnection(std::move(socket), true);
    return true;
  } catch (const std::exception &e) {
    LOG_ERROR << "Dial error: " << e.what();
    return false;
  }
}

bool TCPTransport::ListenAndAccept() {
  try {
    acceptor_.listen();
    LOG_INFO << "TCP transport listening on " << opts_.listenAddr;

    // launch accept loop in separate thread
    std::thread([this]() { startAcceptLoop(); }).detach();
    return true;
  } catch (const std::exception &e) {
    LOG_ERROR << "Listen error: " << e.what();
    return false;
  }
}

void TCPTransport::startAcceptLoop() {
  LOG_INFO << "Starting accept loop";

  while (acceptor_.is_open()) {
    try {
      // wait for and accept new connections
      boost::asio::ip::tcp::socket socket(io_context_);
      acceptor_.accept(socket);
      LOG_DEBUG << "Accepted new connection from: "
                << socket.remote_endpoint().address().to_string();

      // handle inbound connection
      handleConnection(std::move(socket), false);
    } catch (const std::exception &e) {
      if (!acceptor_.is_open()) {
        LOG_INFO << "Acceptor closed, stopping accept loop";
        break;
      }
      LOG_ERROR << "Accept error: " << e.what();
    }
  }
}

void TCPTransport::handleConnection(boost::asio::ip::tcp::socket socket,
                                    bool outbound) {
  try {
    auto peer = std::make_shared<TCPPeer>(std::move(socket), outbound);
    LOG_INFO << "Handling new " << (outbound ? "outbound" : "inbound")
             << " connection from " << peer->RemoteAddr();

    if (!opts_.handshakeFunc(*peer)) {
      LOG_ERROR << "Handshake failed with " << peer->RemoteAddr();
      return;
    }

    {
      std::lock_guard<std::mutex> lock(peersMutex_);
      peers_[peer->RemoteAddr()] = peer;
    }

    if (opts_.OnPeer) {
      opts_.OnPeer(peer);
    }

    std::thread([this, peer]() {
      LOG_INFO << "Starting read loop for " << peer->RemoteAddr();

      try {
        RPC rpc;
        ssize_t bytesRead = opts_.decoder->Decode(peer->socket(), rpc);

        if (bytesRead < 0) {
          // Fatal error or connection closed
          LOG_INFO << "Connection closed to peer: " << peer->RemoteAddr();
          return;
        }

        if (bytesRead == 0) {
          // Temporary error, retry after short delay
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          return;
        }

        if (rpc.isStream()) {
          peer->StartStream();
          LOG_DEBUG << "[" << peer->RemoteAddr()
                    << "] incoming stream, waiting...";
          peer->WaitForStream();
          LOG_DEBUG << "[" << peer->RemoteAddr()
                    << "] stream closed, resuming read loop";
        }

        rpc.setFrom(peer->RemoteAddr());
        rpcChan_->Send(std::move(rpc));

      } catch (const boost::system::system_error &e) {
        LOG_ERROR << "Connection error for " << peer->RemoteAddr() << ": "
                  << e.what();
      }

      LOG_INFO << "Read loop ended for " << peer->RemoteAddr();
      removePeer(peer->RemoteAddr());
    }).detach();

  } catch (const std::exception &e) {
    LOG_ERROR << "Handle connection error: " << e.what();
  }
}

std::shared_ptr<Channel<RPC>> TCPTransport::Consume() {
  LOG_DEBUG << "Returning RPC channel";
  return rpcChan_;
}

bool TCPTransport::Close() {
  // Atomic guard against multiple closes
  bool expected = false;
  if (!closed_.compare_exchange_strong(expected, true)) {
    return true; // Already closed
  }

  LOG_INFO << "Closing TCP Transport on " << opts_.listenAddr;

  try {
    // stop accepting new connections
    if (acceptor_.is_open()) {
      acceptor_.close();
    }
    LOG_DEBUG << "Acceptor closed";

    io_context_.stop();

    // close all peer connections
    std::lock_guard<std::mutex> lock(peersMutex_);

    LOG_INFO << "Closing " << peers_.size() << " peer connections";
    for (auto &[addr, peer] : peers_) {
      LOG_DEBUG << "Closing peer connection: " << addr;
      auto &socket = peer->socket();

      // Shutdown both send and receive operations
      socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

      // Close the socket
      socket.close();
    }

    peers_.clear();
    LOG_INFO << "TCP Transport closed successfully";
    return true;
  } catch (const std::exception &e) {
    LOG_ERROR << "Close error: " << e.what();
    return false;
  }
}

void TCPTransport::removePeer(const std::string &addr) {
  std::lock_guard<std::mutex> lock(peersMutex_);
  peers_.erase(addr);
  LOG_INFO << "Removed peer " << addr << " (remaining peers: " << peers_.size()
           << ")";
}

} // namespace network
} // namespace dfs
