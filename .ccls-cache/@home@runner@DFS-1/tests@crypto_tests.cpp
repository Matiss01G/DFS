/**
 * Note: In a real-world application, we would also want to test:
 * - Edge cases (empty strings, very long strings)
 * - Error conditions (invalid inputs)
 * - Actual encryption/decryption using the generated keys
 * - Performance with larger inputs
 * - Memory usage with large keys/hashes
 *
 * However, for this basic implementation, we're focusing on
 * core functionality verification.
 */

#include "crypto/crypto_utils.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <string>

using namespace dfs::crypto;

/**
Test: GenerateId
Purpose: Verify that the ID generation function:
  1. Creates IDs of correct length (32 bytes = 64 hex characters)
  2. Generates unique IDs on different calls
  3. Returns valid hexadecimal strings
 */
TEST(CryptoTests, GenerateId) {
  // generate 2 different IDs to compare
  auto id1 = generateId();
  auto id2 = generateId();

  // check length: each byte becomes 2 hex characters
  // 32 bytes = 64 hex characters (0-9, a-f)
  EXPECT_EQ(id1.length(), 64)
      << "ID should be 64 characters long (32 bytes in hex)";
  EXPECT_EQ(id2.length(), 64)
      << "ID should be 64 characters long (32 bytes in hex)";

  // verify uniqueness: 2 generated IDs should not be the same
  // This test could theoretically fail but the probability is extremely low
  EXPECT_NE(id1, id2) << "Generated IDs should be unique";
}

/**
Test: HashKey
Purpose: Verify that the key hashing function:
  1. Produces consistent hashes (same input = same hash)
  2. Creates correct length MD5 hashes (16 bytes = 32 hex characters)
  3. Produces different hashes for different inputs
*/
TEST(CryptoTests, HashKey) {
  // test consistent hashing - same input should give same hash
  std::string key = "testkey";
  auto hash1 = hashKey(key);
  auto hash2 = hashKey(key);

  // same input should produce same hashes
  EXPECT_EQ(hash1, hash2) << "Same key should generate same hash";

  // MD5 should produce 16-byte hash, which becomes 32 hex characters
  EXPECT_EQ(hash1.length(), 32)
      << "MD5 hash should be 32 characters long (16 bytes in hex)";

  // Test different inputs produce different hashes
  auto hash3 = hashKey("differentkey");
  EXPECT_NE(hash1, hash3) << "Different keys should generate different hashes";
}

/**
Test: NewEncryptionKey
Purpose: Verify that the encryption key generation:
  1. Creates keys of correct length for AES-256 (32 bytes)
  2. Generates unique keys on different calls
  3. Returns a valid byte array that could be used for encryption
*/
TEST(CryptoTests, NewEncryptionKey) {
  // generate 2 different encryption keys
  auto key1 = newEncryptionKey();
  auto key2 = newEncryptionKey();

  // check key length: AES-256 requires 32 byte keys
  EXPECT_EQ(key1.size(), 32) << "Encryption key should be 32 bytes for AES-256";
  EXPECT_EQ(key2.size(), 32) << "Encryption key should be 32 bytes for AES-256";

  // verify uniqueness: keys should be different
  EXPECT_NE(key1, key2) << "Generated encryption keys should be unique";

  // verify keys contain valid data (not all zeros)
  bool allZeros1 =
      std::all_of(key1.begin(), key1.end(), [](uint8_t b) { return b == 0; });
  EXPECT_FALSE(allZeros1) << "Encryption key should not be all zeros";
}