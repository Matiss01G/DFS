#include "storage/store.h"
#include "logging/logging.hpp"
#include <iostream>
#include <system_error>

namespace dfs {
namespace storage {

Store::Store(StoreOpts opts) : opts_(std::move(opts)) {
  LOG_INFO << "Initializing Store with root directory: " << opts_.root;
  std::filesystem::create_directories(opts_.root);
}

std::int64_t Store::Write(const std::string &id, const std::string &key,
                          std::istream &data) {
  // Transform the key into a path using the configured transform function
  PathKey pathKey = opts_.pathTransformFunc(key);

  // Get full filesystem path and open output file
  auto fullPath = getFullPath(id, pathKey);

  LOG_INFO << "Writing file - Key: " << key;
  LOG_INFO << "Transformed path: " << pathKey.fullPath();
  LOG_INFO << "Full filesystem path: " << fullPath.string();

  auto outFile = openFileForWriting(fullPath);
  if (!outFile || !outFile->is_open()) {
    LOG_ERROR << "Failed to open file for writing: " << fullPath.string();
    return -1;
  }

  // Copy data from input stream to file in chunks
  std::int64_t bytesWritten = 0;
  char buffer[8192];
  while (data.read(buffer, sizeof(buffer))) {
    bytesWritten += data.gcount();
    outFile->write(buffer, data.gcount());
  }

  // Handle any remaining partial buffer
  if (data.gcount() > 0) {
    bytesWritten += data.gcount();
    outFile->write(buffer, data.gcount());
  }

  LOG_INFO << "Successfully wrote " << bytesWritten << " bytes to "
           << fullPath.string();
  return bytesWritten;
}

ReadResults Store::Read(const std::string &id, const std::string &key) {
  ReadResults result;
  PathKey pathKey = opts_.pathTransformFunc(key);
  auto fullPath = getFullPath(id, pathKey);

  LOG_INFO << "Reading file - Key: " << key;
  LOG_INFO << "Transformed path: " << pathKey.fullPath();
  LOG_INFO << "Full filesystem path: " << fullPath.string();

  if (!std::filesystem::exists(fullPath)) {
    LOG_ERROR << "File not found: " << fullPath.string();
    return result;
  }

  result.size = std::filesystem::file_size(fullPath);
  result.stream = std::make_unique<std::ifstream>(fullPath, std::ios::binary);

  if (!result.stream->good()) {
    LOG_ERROR << "Failed to open file for reading: " << fullPath.string();
    result.stream = nullptr;
    return result;
  }

  LOG_INFO << "Successfully opened file, size: " << result.size << " bytes";
  return result;
}

bool Store::Has(const std::string &id, const std::string &key) const {
  PathKey pathKey = opts_.pathTransformFunc(key);
  auto fullPath = getFullPath(id, pathKey);

  bool exists = std::filesystem::exists(fullPath);
  LOG_DEBUG << "Checking existence - Key: " << key
            << ", Path: " << fullPath.string()
            << ", Exists: " << (exists ? "yes" : "no");

  return exists;
}

bool Store::Delete(const std::string &id, const std::string &key) {
  PathKey pathKey = opts_.pathTransformFunc(key);
  auto firstDir =
      std::filesystem::path(opts_.root) / id / pathKey.firstPathName();

  LOG_INFO << "Deleting directory tree - Key: " << key;
  LOG_INFO << "Directory to remove: " << firstDir.string();

  std::error_code ec;
  auto removed = std::filesystem::remove_all(firstDir, ec);

  if (ec) {
    LOG_ERROR << "Failed to delete directory: " << firstDir.string()
              << ", Error: " << ec.message();
    return false;
  }

  LOG_INFO << "Successfully removed " << removed << " files/directories";
  return removed > 0;
}

bool Store::Clear() {
  LOG_INFO << "Clearing all files from store root: " << opts_.root;

  std::error_code ec;
  auto removed = std::filesystem::remove_all(opts_.root, ec);

  if (ec) {
    LOG_ERROR << "Failed to clear store: " << ec.message();
    return false;
  }

  LOG_INFO << "Successfully removed " << removed << " files/directories";
  return removed > 0;
}

std::filesystem::path Store::getFullPath(const std::string &id,
                                         const PathKey &pathKey) const {
  auto path = std::filesystem::path(opts_.root) / id / pathKey.fullPath();
  LOG_DEBUG << "Constructed full path - ID: " << id
            << ", PathKey: " << pathKey.fullPath()
            << ", Full path: " << path.string();
  return path;
}

std::unique_ptr<std::ofstream>
Store::openFileForWriting(const std::filesystem::path &path) {
  LOG_DEBUG << "Creating directories for path: " << path.string();
  std::filesystem::create_directories(path.parent_path());

  auto file = std::make_unique<std::ofstream>(path, std::ios::binary);
  if (!file->is_open()) {
    LOG_ERROR << "Failed to open file for writing: " << path.string();
  }
  return file;
}

} // namespace storage
} // namespace dfs