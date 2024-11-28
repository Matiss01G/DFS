#include "storage/store.h"
#include <system_error>

namespace dfs {
namespace storage {

// constructor uses default root=ggnetwork, DefaultPathTransformFunc if no
// options provided
Store::Store(StoreOpts opts) : opts_(std::move(opts)) {
  std::filesystem::create_directories(opts_.root);
}

// writes data from input stream to a file in the store
// returns number of bytes written or -1 for error
std::int64_t Store::Write(const std::string &id, const std::string &key,
                          std::istream &data) {
  // transform key into path using configured pathTransformFunc
  PathKey pathKey = opts_.pathTransformFunc(key);

  // get full path, including root directory and node ID
  auto fullPath = getFullPath(id, pathKey);

  // open file for writing, creating directories as needed                                                                      
  auto outFile = openFileForWriting(fullPath);
  if (!outFile || !outFile->is_open()) {
    return -1;
  }

  // copy data from input stream into file using buffered I/O for efficiency
  std::int64_t bytesWritten = 0;
  char buffer[8192];
  while (data.read(buffer, sizeof(buffer))) {
    bytesWritten += data.gcount();
    outFile->write(buffer, data.gcount());
  }

  // handle any remaining bytes (last chunk might be smaller than buffer)
  if (data.gcount() > 0) {
    bytesWritten += data.gcount();
    outFile->write(buffer, data.gcount());
  }

  return bytesWritten;
}

// Reads a file from the store
ReadResults Store::Read(const std::string& id, const std::string& key) {
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
  auto firstDir = std::filesystem::path(opts_.root) / id / pathKey.firstPathName();

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