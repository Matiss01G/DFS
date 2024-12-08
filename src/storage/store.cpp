#include "storage/store.h"
#include <system_error>

namespace dfs {
namespace storage {

// constructor uses default root=ggnetwork, DefaultPathTransformFunc if no
// options provided
Store::Store(StoreOpts opts) : opts_(std::move(opts)) {
  std::filesystem::create_directories(opts_.root);
}

std::int64_t Store::Write(const std::string &id, const std::string &hashedKey,
                          std::istream &data) {
  // Create PathKey directly from hashed key without additional hashing
  PathKey pathKey(hashedKey, hashedKey);
  return WriteHelp(id, pathKey, data);
}

std::int64_t Store::HashAndWrite(const std::string &id, const std::string &key,
                                 std::istream &data) {
  // Use path transform function which includes hashing
  PathKey pathKey = opts_.pathTransformFunc(key);
  return WriteHelp(id, pathKey, data);
}

// Common implementation for both write methods
std::int64_t Store::WriteHelp(const std::string &id, const PathKey &pathKey,
                              std::istream &data) {
  auto fullPath = getFullPath(id, pathKey);
  auto outFile = openFileForWriting(fullPath);
  if (!outFile || !outFile->is_open()) {
    return -1;
  }

  std::int64_t bytesWritten = 0;
  char buffer[8192];
  while (data.read(buffer, sizeof(buffer))) {
    bytesWritten += data.gcount();
    outFile->write(buffer, data.gcount());
  }

  if (data.gcount() > 0) {
    bytesWritten += data.gcount();
    outFile->write(buffer, data.gcount());
  }
  return bytesWritten;
}

// Reads a file from the store
ReadResults Store::Read(const std::string &id, const std::string &key) {
  ReadResults result;

  PathKey pathKey = opts_.pathTransformFunc(key);
  auto fullPath = getFullPath(id, pathKey);

  if (!std::filesystem::exists(fullPath)) {
    return result;
  }

  result.size = std::filesystem::file_size(fullPath);
  result.stream = std::make_unique<std::ifstream>(fullPath, std::ios::binary);

  return result;
}

// checks if a file exists in the store
bool Store::Has(const std::string &id, const std::string &key) const {
  PathKey pathKey = opts_.pathTransformFunc(key);
  return std::filesystem::exists(getFullPath(id, pathKey));
}

// deletes a file and its parent directory structure
bool Store::Delete(const std::string &id, const std::string &key) {
  PathKey pathKey = opts_.pathTransformFunc(key);

  // get the first directory in the path to remove entire subtree
  auto firstDir =
      std::filesystem::path(opts_.root) / id / pathKey.firstPathName();

  std::error_code ec;
  return std::filesystem::remove_all(firstDir, ec) > 0;
}

// clears all files and directories in the store's root
bool Store::Clear() {
  std::error_code ec;
  return std::filesystem::remove_all(opts_.root, ec) > 0;
}

// helper function to construct full filesystem path for a file
// combines root dir + node ID + transformed path + filename
std::filesystem::path Store::getFullPath(const std::string &id,
                                         const PathKey &pathKey) const {
  return std::filesystem::path(opts_.root) / id / pathKey.fullPath();
}

// helper function to prepare file for writing
// creates all necessary parent directories, opens file in binary mode, returns
// pointer to output file stream or nullptr on error
std::unique_ptr<std::ofstream>
Store::openFileForWriting(const std::filesystem::path &path) {
  // create all parent directories first
  std::filesystem::create_directories(path.parent_path());

  auto file = std::make_unique<std::ofstream>(path, std::ios::binary);
  return file;
}

} // namespace storage
} // namespace dfs