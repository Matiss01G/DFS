#include "storage/path_key.h"
#include "crypto/crypto_utils.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace dfs {
namespace storage {

// Constructor takes ownership of paths using move semantics 
// for better performance
PathKey::PathKey(std::string pathName, std::string filename)
    : pathName_(std::move(pathName)), filename_(std::move(filename)) {}

// Returns the first directory in the path hierarchy
// Example 1: pathName_ = "68044/29f74/181a6". pos will be 5 (index of first
// '/'). Returns: "68044" Example 2: pathName_ = "simple". no '/' found.
// Returns: "simple"
std::string PathKey::firstPathName() const {
  auto pos = pathName_.find('/');
  return (pos == std::string::npos) ? pathName_ : pathName_.substr(0, pos);
}

// Combines the path and filename to create the complete file path
std::string PathKey::fullPath() const { return pathName_ + "/" + filename_; }


  

// Implements Content-Addressed Storage path generation
// Takes key and returns PathKey with hierarchical directory structure based on
// content hash and full hash as filename
PathKey CASPathTransformFunc(const std::string &key) {
  std::string hashStr = crypto::hashKey(key);

  const size_t blockSize = 5;
  std::vector<std::string> paths;

  // Example: hash "68044297417481a63c50c... Becomes directories: ["68044",
  // "29741", "7481a", "63c50", ...]
  for (size_t i = 0; i < hashStr.length(); i += blockSize) {
    if (i + blockSize <= hashStr.length()) {
      paths.push_back(hashStr.substr(i, blockSize));
    }
  }

  // Join the path components with '/' to create the directory structure
  std::string pathName;
  for (size_t i = 0; i < paths.size(); i++) {
    if (i > 0) {
      pathName += '/';
    }
    pathName += paths[i];
  }

  return PathKey(pathName, hashStr);
}

// DefaultPathTransformFunc provides a simple 1:1 mapping
// Example: key "test.txt" becomes path "test.txt/test.txt"
PathKey DefaultPathTransformFunc(const std::string &key) {
  return PathKey(key, key);
}

} // namespace storage
} // namespace dfs