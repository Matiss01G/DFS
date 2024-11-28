#pragma once

/**
 * @file crypto_utils.h
 * @brief Cryptographic utilities for the distributed file system
 * 
 * This file provides core cryptographic operations needed by the DFS including:
 * - Generating random IDs for nodes and files
 * - Hashing keys for content addressing
 * - Creating encryption keys
 * - Setting up encryption/decryption streams for secure file transfer
 * 
 * The implementation uses the Crypto++ library for all cryptographic operations
 * and follows modern C++ practices for memory safety and RAII.
 */

#include <string>
#include <vector>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/md5.h>

namespace dfs {
namespace crypto {

// Generates a random 32-byte ID and returns it as hex string
std::string generateId();

// Creates MD5 hash of input key
std::string hashKey(const std::string& key);

// Generates new 32-byte AES encryption key
std::vector<uint8_t> newEncryptionKey();

// Utility type for encryption/decryption streams
using CryptoStream = std::shared_ptr<CryptoPP::StreamTransformation>;

// Structure to hold both the stream and its IV
struct EncryptionStreamInfo {
    CryptoStream stream;
    std::vector<uint8_t> iv;
};

// Creates encryption stream and returns it along with the IV
EncryptionStreamInfo createEncryptStream(const std::vector<uint8_t>& key);

// Creates decryption stream using the provided IV
CryptoStream createDecryptStream(const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);

// Encrypts data from source stream to destination stream
std::int64_t copyEncrypt(const std::vector<uint8_t>& key,
                        std::istream& src,
                        std::ostream& dst);

// Decrypts data from source stream to destination stream
std::int64_t copyDecrypt(const std::vector<uint8_t>& key,
                        std::istream& src,
                        std::ostream& dst);

// Helper function to copy data between streams while encrypting/decrypting
std::int64_t copyStream(CryptoStream& stream,
                        std::istream& src,
                        std::ostream& dst);
} 
}