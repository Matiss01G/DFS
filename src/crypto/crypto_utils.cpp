#include "crypto/crypto_utils.h"
#include "logging/logging.hpp"
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

  const size_t BUFFER_SIZE = 8192;
  std::vector<uint8_t> buffer(BUFFER_SIZE);    // input buffer
  std::vector<uint8_t> processed(BUFFER_SIZE); // output buffer
  std::int64_t totalBytes = 0;

  LOG_DEBUG << "Starting copyStream";

  while (src.good()) {
    src.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
    auto bytesRead = src.gcount();

    LOG_DEBUG << "Read " << bytesRead << " bytes from source stream";

    if (bytesRead > 0) {
      stream->ProcessData(processed.data(), buffer.data(), bytesRead);

      // Log first few bytes before writing
      if (bytesRead > 4) {
        LOG_DEBUG << "First 4 bytes being written: " << std::hex
                  << static_cast<int>(processed[0]) << " "
                  << static_cast<int>(processed[1]) << " "
                  << static_cast<int>(processed[2]) << " "
                  << static_cast<int>(processed[3]) << std::dec;
      }

      dst.write(reinterpret_cast<const char *>(processed.data()), bytesRead);
      totalBytes += bytesRead;
      LOG_DEBUG << "Processed and wrote " << bytesRead << " bytes";
    }
  }
  LOG_DEBUG << "copyStream completed. Total bytes: " << totalBytes;
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

  LOG_DEBUG << "=== ENCRYPTION START ===";
  LOG_DEBUG << "Starting encryption with key size: " << key.size();

  // Log key bytes in detail
  std::stringstream keyHex;
  for (size_t i = 0; i < key.size(); i++) {
    keyHex << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(key[i]) << " ";
  }
  LOG_DEBUG << "Full encryption key: " << keyHex.str();

  // Create encryption stream and get IV
  auto encInfo = createEncryptStream(key);

  // Log IV in detail
  std::stringstream ivHex;
  for (size_t i = 0; i < encInfo.iv.size(); i++) {
    ivHex << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(encInfo.iv[i]) << " ";
  }
  LOG_DEBUG << "Full IV: " << ivHex.str();

  // Log original content
  std::stringstream contentCopy;
  contentCopy << src.rdbuf();
  std::string originalContent = contentCopy.str();
  LOG_DEBUG << "Original content hex:";
  for (size_t i = 0; i < originalContent.size(); i++) {
    LOG_DEBUG << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(static_cast<uint8_t>(originalContent[i]))
              << " ";
  }

  // Reset src stream
  src.clear();
  src.seekg(0);

  // write IV at start of output
  LOG_DEBUG << "Writing IV of size " << encInfo.iv.size()
            << " to output stream";
  dst.write(reinterpret_cast<const char *>(encInfo.iv.data()),
            encInfo.iv.size());

  auto bytesProcessed = detail::copyStream(encInfo.stream, src, dst);
  if (bytesProcessed < 0)
    return -1;

  LOG_DEBUG << "=== ENCRYPTION COMPLETE ===";

  LOG_DEBUG << "Encryption complete. Processed bytes: " << bytesProcessed
            << " Total with IV: " << (bytesProcessed + encInfo.iv.size());

  return bytesProcessed + encInfo.iv.size();
}

std::int64_t copyDecrypt(const std::vector<uint8_t> &key, std::istream &src,
                         std::ostream &dst) {
  LOG_DEBUG << "=== DECRYPTION START ===";
  LOG_DEBUG << "Starting decryption with key size: " << key.size();

  // Log key bytes in detail
  std::stringstream keyHex;
  for (size_t i = 0; i < key.size(); i++) {
    keyHex << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(key[i]) << " ";
  }
  LOG_DEBUG << "Full decryption key: " << keyHex.str();

  // Read IV from start of input
  std::vector<uint8_t> iv(CryptoPP::AES::BLOCKSIZE);
  src.read(reinterpret_cast<char *>(iv.data()), iv.size());

  if (src.gcount() != iv.size()) {
    LOG_ERROR << "Failed to read full IV. Expected " << iv.size()
              << " bytes but got " << src.gcount();
    return -1;
  }

  // Log IV in detail
  std::stringstream ivHex;
  for (size_t i = 0; i < iv.size(); i++) {
    ivHex << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(iv[i]) << " ";
  }
  LOG_DEBUG << "Read IV: " << ivHex.str();

  // Create decryption stream
  auto decStream = createDecryptStream(key, iv);

  // Read only the encrypted portion (not including IV)
  std::vector<uint8_t> encryptedData;
  std::vector<uint8_t> decryptedData;

  // Calculate encrypted data size
  auto currentPos = src.tellg();
  src.seekg(0, std::ios::end);
  auto length = src.tellg();
  auto dataSize = length - currentPos;
  src.seekg(currentPos);

  LOG_DEBUG << "Encrypted data size (excluding IV): " << dataSize;

  // Read encrypted data
  encryptedData.resize(dataSize);
  src.read(reinterpret_cast<char *>(encryptedData.data()), dataSize);

  if (dataSize > 4) {
    LOG_DEBUG << "First 4 bytes of encrypted data: " << std::hex
              << static_cast<int>(encryptedData[0]) << " "
              << static_cast<int>(encryptedData[1]) << " "
              << static_cast<int>(encryptedData[2]) << " "
              << static_cast<int>(encryptedData[3]) << std::dec;
  }

  // Prepare output buffer
  decryptedData.resize(dataSize);

  // Decrypt in one shot
  decStream->ProcessData(decryptedData.data(), encryptedData.data(), dataSize);

  if (dataSize > 4) {
    LOG_DEBUG << "First 4 bytes of decrypted data: " << std::hex
              << static_cast<int>(decryptedData[0]) << " "
              << static_cast<int>(decryptedData[1]) << " "
              << static_cast<int>(decryptedData[2]) << " "
              << static_cast<int>(decryptedData[3]) << std::dec;
  }

  // Write decrypted data
  dst.write(reinterpret_cast<const char *>(decryptedData.data()), dataSize);

  // Log decrypted content
  std::string decryptedContent(decryptedData.begin(), decryptedData.end());
  LOG_DEBUG << "Decrypted content: [" << decryptedContent << "]";

  LOG_DEBUG << "=== DECRYPTION COMPLETE ===";
  LOG_DEBUG << "Processed " << dataSize << " bytes";

  return dataSize;
}

} // namespace crypto
} // namespace dfs