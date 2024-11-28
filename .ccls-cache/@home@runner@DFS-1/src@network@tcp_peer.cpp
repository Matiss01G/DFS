#include "network/tcp_peer.h"
#include <boost/asio.hpp>

namespace dfs {
namespace network {

// takes pre-connected socket and flag indicating inbound/outbound
TCPPeer::TCPPeer(boost::asio::ip::tcp::socket socket, bool outbound)
  : socket_(std::move(socket)) // takes ownership of socket
  , outbound_(outbound)
  {}

// sends raw data to peer over tcp connection
// while data vector must be sent - partial sends not supported
bool TCPPeer::Send(const std::vector<uint8_t>& data){
  try{
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(data), ec);
    return !ec;
  } catch (const boost::system::system_error& e){
    return false;
  }
}

// marks the end of a stream operation 
void TCPPeer::CloseStream(){
  {
  std::lock_guard<std::mutex> lock(stream_mutex_);
  stream_complete_ = true;
  }
  // wake up any threads waiting for stream completion
  stream_cv_.notify_all();
}

// marks start of new operation
void TCPPeer::StartStream(){
  std::lock_guard<std::mutex> lock(stream_mutex_);
  stream_complete_ = false;
}

// blocks calling thread until current stream operation is complete
void TCPPeer::WaitForStream(){
  std::unique_lock<std::mutex> lock(stream_mutex_);
  stream_cv_.wait(lock, [this]() { return stream_complete_; });
}

// gets IP address and port # if connected peer from socket
// returns empty string if error occurs
std::string TCPPeer::RemoteAddr() const {
  try {
    return socket_.remote_endpoint().address().to_string() + ":" +  
           std::to_string(socket_.remote_endpoint().port());
  } catch (const boost::system::system_error& e) {
    return "";
  }
}

}
}  