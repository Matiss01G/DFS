// include/network/peer_whitelist.h
#pragma once

#include <string>
#include <unordered_set>

namespace dfs {
namespace network {

class PeerWhitelist {
public:
  // Constructor no longer needs to take peers - uses predefined set
  PeerWhitelist() = default;

  // Check if a peer is whitelisted
  bool IsWhitelisted(const std::string &addr) const;

private:
  // Predefined set of allowed peers
  static const std::unordered_set<std::string> ALLOWED_PEERS;
};

} // namespace network
} // namespace dfs