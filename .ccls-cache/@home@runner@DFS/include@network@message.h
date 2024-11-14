#pragma once

/**
 * @file message.h
 * @brief Network message types and RPC structure for the distributed file system
 * 
 * This file defines the core message types and structures used for network
 * communication between nodes in the distributed file system. It includes:
 * - Message type constants for different kinds of network messages
 * - RPC structure for encapsulating messages
 * - Serialization interface for network transmission
 */

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

namespace dfs {
namespace network{

// message types used in the wire protocol. Identify what kind of data 
//  is being sent in between nodes
enum class MessageType : uint8_t {
  IncomingMessage = 0x1,
  IncomingStream = 0x2
};     

class RPC {
public:
  RPC() = default;
  
  // creates a new RPC message with the given sender, data, and stream flag
  RPC(std::string from, std::vector<uint8_t> payload, bool stream = false)
      : from_(std::move(from))
      , payload_(std::move(payload))
      , stream_(stream)
  {}
  
  // getters for message fields
  const std::string& from() const { return from_; }
  const std::vector<uint8_t>& payload() const { return payload_; }
  bool isStream() const { return stream_; }

  // setters for message fields
  void setFrom(std::string from) { from_ = std::move(from); }
  void setPayload(std::vector<uint8_t> payload) { payload_ = std::move(payload); }
  void setStream(bool stream) { stream_ = stream; }

  // writes the RPC message to given output stream in format that 
  // can be sent over network
  bool serialize(std::ostream& os) const;

  // reads RPC message from given input stream, populating this object
  bool deserialize(std::istream& is);

private:
  std::string from_;
  std::vector<uint8_t> payload_;
  bool stream_ = false;
};
  
}
}