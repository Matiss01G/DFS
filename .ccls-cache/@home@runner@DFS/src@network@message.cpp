#include "network/message.h"
#include <cstring>

namespace dfs {
namespace network {

bool RPC::serialize(std::ostream& os) const{
  // first write the sender address length and data
  uint32_t fromLen = static_cast<uint32_t>(from_.length());
  os.write(reinterpret_cast<const char*>(&fromLen), sizeof(fromLen));
  os.write(from_.data(), fromLen);

  // then, write the payload size and data
  uint32_t payloadLen = static_cast<uint32_t>(payload_.size());
  os.write(reinterpret_cast<const char*>(&payloadLen), sizeof(payloadLen));
  os.write(reinterpret_cast<const char*>(payload_.data()), payloadLen);

  // write the stream flag
  os.write(reinterpret_cast<const char*>(&stream_), sizeof(stream_));

  return os.good();
}


bool RPC::deserialize(std::istream& is){
  // read the sender address length
  uint32_t fromLen = 0;
  is.read(reinterpret_cast<char*>(&fromLen), sizeof(fromLen));
  if (!is.good()) return false;

  // read the sender address data
  from_.resize(fromLen);
  is.read(&from_[0], fromLen);
  if (!is.good()) return false;

  // read the payload length
  uint32_t payloadLen = 0;
  is.read(reinterpret_cast<char*>(&payloadLen), sizeof(payloadLen));
  if (!is.good()) return false;

  // read the payload data
  payload_.resize(payloadLen);
  is.read(reinterpret_cast<char*>(payload_.data()), payloadLen);
  if(!is.good()) return false;

  // read the stream flag
  is.read(reinterpret_cast<char*>(&stream_), sizeof(stream_));

  return is.good();
}
  
}
}