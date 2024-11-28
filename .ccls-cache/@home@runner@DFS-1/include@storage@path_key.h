#pragma once

/**
  @file path_key.h
  @brief Handles path generation and transformation for the distributed file system
  
  The PathKey class and associated functions manage how files are stored and 
  organized in the system. It supports two main strategies:
  1. Content-addressed storage (CAS) where file paths are generated from content hashes
  2. Direct mapping where keys map directly to paths
  
  This component is crucial for:
  - Organizing files in a content-addressable way
  - Preventing path collisions
  - Maintaining consistent file organization across nodes
  - Supporting efficient file lookup and retrieval
 */

#include <string>
#include <vector>
#include <functional>

namespace dfs{
namespace storage{

class PathKey{
public:
  PathKey(std::string pathName, std::string filename);

  // Get the first component of the path
  std::string firstPathName() const;

  // get full path (pathname/filename)
  std::string fullPath() const;

  // getters:
  const std::string& pathName() const { return pathName_; }
  const std::string& filename() const {return filename_; }

private:
  std::string pathName_;
  std::string filename_;

};

// function type for path transformation
using PathTransformFunc = std::function<PathKey(const std::string&)>;

// default implementation using content-addressing storage (CAS)
PathKey CASPathTransformFunc(const std::string& key);

// simple pass-through transform for testing
PathKey DefaultPathTransformFunc(const std::string& key);
  
}
}