#include "crypto/crypto_utils.h"
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <iostream>
#include <string>
#include <vector>

namespace dfs {
namespace crypto {
namespace detail {
std::int64_t copyStream(CryptoStream &stream, std::istream &src,
                        std::ostream &dst) {

  std::vector<uint8_t> buffer(8192);    // input buffer
  std::vector<uint8_t> processed(8192); // output buffer
  std::int64_t totalBytes = 0;

  while (src.good()) {
    src.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
    auto bytesRead = src.gcount();

    if (bytesRead > 0) {
      stream->ProcessData(processed.data(), buffer.data(), bytesRead);
      dst.write(reinterpret_cast<const char *>(processed.data()), bytesRead);
      totalBytes += bytesRead;
    }
  }
  return totalBytes;
}

} // namespace detail

std::string generateId() {
  // create 32 byte random ID
  CryptoPP::AutoSeededRandomPool rng;
  std::vector<uint8_t> buffer(32);
  rng.GenerateBlock(buffer.data(), buffer.size());

  // convert to hex string
  std::string hexString;
  CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(hexString));
  encoder.Put(buffer.data(), buffer.size());
  encoder.MessageEnd();
  return hexString;
}

std::string hashKey(const std::string &key) {
  // create Md5 hash
  CryptoPP::MD5 hash;
  std::string digest;
  // pipeline of operations:
  CryptoPP::StringSource s(
      key, true,
      new CryptoPP::HashFilter(
          hash,                     // step 1: calculate MD5 hash
          new CryptoPP::HexEncoder( // step 2: convert hash to hex
              new CryptoPP::StringSink(digest)))); // step 3: store in digest
  return digest;
}

std::vector<uint8_t> newEncryptionKey() {
  // generate random 32-byte key for AES-256
  CryptoPP::AutoSeededRandomPool rng;
  std::vector<uint8_t> key(32);
  rng.GenerateBlock(key.data(), key.size());
  return key;
}

EncryptionStreamInfo createEncryptStream(const std::vector<uint8_t> &key) {
  // Create AES encryption in CTR mode
  auto aes = std::make_shared<CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption>();
  // Generate random IV
  CryptoPP::AutoSeededRandomPool rng;
  std::vector<uint8_t> iv(CryptoPP::AES::BLOCKSIZE);
  rng.GenerateBlock(iv.data(), iv.size());
  // Initialize cipher
  aes->SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
  return EncryptionStreamInfo{aes, iv}; // Return struct with both pieces
}

CryptoStream createDecryptStream(const std::vector<uint8_t> &key,
                                 const std::vector<uint8_t> &iv) {
  // Create AES decryption in CTR mode
  auto aes = std::make_shared<CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption>();
  // Use provided IV from encryption
  aes->SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
  return aes;
}

std::int64_t copyEncrypt(const std::vector<uint8_t> &key, std::istream &src,
                         std::ostream &dst) {
  // Create encryption stream and get IV
  auto encInfo = createEncryptStream(key);

  // write IV at start of output
  dst.write(reinterpret_cast<const char *>(encInfo.iv.data()),
            encInfo.iv.size());

  auto bytesProcessed = detail::copyStream(encInfo.stream, src, dst);
  if (bytesProcessed < 0)
    return -1;

  return bytesProcessed + encInfo.iv.size();
}

std::int64_t copyDecrypt(const std::vector<uint8_t> &key, std::istream &src,
                         std::ostream &dst) {
  // Read IV from start of input
  std::vector<uint8_t> iv(CryptoPP::AES::BLOCKSIZE);
  src.read(reinterpret_cast<char *>(iv.data()), iv.size());
  if (src.gcount() != iv.size())
    return -1;

  // Create decryption stream
  auto decStream = createDecryptStream(key, iv);

  // Process the data
  return detail::copyStream(decStream, src, dst);
}

} // namespace crypto
} // namespace dfs