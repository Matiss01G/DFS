#pragma once

/**
 * @file store.h
 * @brief File storage management for the distributed file system
 *
 * The Store class handles all file operations including:
 * - Writing files to disk with proper path transformations
 * - Reading files from disk
 * - Checking file existence
 * - Deleting files
 * - Managing the storage directory structure
 *
 * It works in conjunction with PathKey to maintain a consistent
 * file organization across the distributed system.
 */

#include "storage/path_key.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace dfs {
namespace storage {

struct StoreOpts {
  std::string root = "ggnetwork";
  PathTransformFunc pathTransformFunc = DefaultPathTransformFunc;
};

struct ReadResults {
  std::int64_t size = 0;
  std::unique_ptr<std::istream> stream;
  bool valid() const {return stream != nullptr;}
};

class Store {
public:
  explicit Store(StoreOpts opts = StoreOpts{});

  // write data from stream to file system
  std::int64_t Write(const std::string &id, const std::string &key,
                    std::istream &data);

  // read data from file sys into stream.
  ReadResults Read(const std::string &id, const std::string &key);

  // check if a file exists
  bool Has(const std::string &id, const std::string &key) const;

  // delete a file
  bool Delete(const std::string &id, const std::string &key);

  // clear all files
  bool Clear();

private:
  StoreOpts opts_;

  // construct full path for a file, including root directory
  std::filesystem::path getFullPath(const std::string &id,
                                    const PathKey &pathKey) const;

  // opens a file for writing, creating directories as needed
  std::unique_ptr<std::ofstream>
  openFileForWriting(const std::filesystem::path &path);
};

} // namespace storage
} // namespace dfs