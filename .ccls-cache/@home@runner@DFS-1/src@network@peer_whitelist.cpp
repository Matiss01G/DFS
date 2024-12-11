// src/network/peer_whitelist.cpp
#include "network/peer_whitelist.h"
#include <iostream>

namespace dfs {
namespace network {

// Define the static set of allowed peers
const std::unordered_set<std::string> PeerWhitelist::ALLOWED_PEERS = {
    "127.0.0.1:58123", "127.0.0.1:58124"};

bool PeerWhitelist::IsWhitelisted(const std::string &addr) const {
  bool allowed = ALLOWED_PEERS.find(addr) != ALLOWED_PEERS.end();
  if (!allowed) {
    std::cout << "Peer " << addr << " not in whitelist" << std::endl;
  }
  return allowed;
}

} // namespace network
} // namespace dfs