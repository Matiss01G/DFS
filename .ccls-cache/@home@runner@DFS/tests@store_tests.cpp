#include "storage/store.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace dfs::storage;

class StoreTest : public ::testing::Test {
protected:
  void SetUp() override {
    // create store with CAS path transformation for testing
    StoreOpts opts;
    opts.root = "test_network";
    opts.pathTransformFunc = CASPathTransformFunc;
    store = std::make_unique<Store>(opts);
  }

  void TearDown() override {
    // clean up all test files
    store->Clear();
  }

  std::unique_ptr<Store> store;
};

/**
  Test basic write and read operations.
  Verifies that:
  1. We can write data to a file
  2. The file exists after writing
  3. We can read back the exact same data
*/
TEST_F(StoreTest, WriteAndReadFile) {
  const std::string testId = "node1";
  const std::string testKey = "test_file.txt";
  const std::string testData = "hello, this is test data!";

  // create string stream with test data
  std::istringstream input(testData);

  // write the data and verify bytes were written
  auto bytesWritten = store->Write(testId, testKey, input);
  EXPECT_GT(bytesWritten, 0);

  // check Has() correctly reports the file exists
  EXPECT_TRUE(store->Has(testId, testKey));

  // read back the data
  auto [size, stream] = store->Read(testId, testKey);
  EXPECT_GT(size, 0);
  ASSERT_TRUE(stream != nullptr);

  // verify the data matches what we wrote
  std::string readData;
  std::getline(*stream, readData);
  EXPECT_EQ(readData, testData);
}

/**
 * Test file deletion.
 * Verifies that:
 * 1. File exists after writing
 * 2. Delete operation succeeds
 * 3. File no longer exists after deletion
 */
TEST_F(StoreTest, DeleteFile) {
  const std::string testId = "node1";
  const std::string testKey = "delete_test.txt";
  const std::string testData = "Delete me!";

  // first write a file
  std::istringstream input(testData);
  store->Write(testId, testKey, input);
  EXPECT_TRUE(store->Has(testId, testKey));

  // delete should succeed
  EXPECT_TRUE(store->Delete(testId, testKey));

  // file should no longer exist
  EXPECT_FALSE(store->Has(testId, testKey));
}

/**
 * Test handling of multiple files.
 * Verifies that:
 * 1. Multiple files can be written
 * 2. Each file exists independently
 * 3. Each file's content is correctly maintained
 * This tests the Store's ability to handle multiple files
 * and keep their contents separate.
 */
TEST_F(StoreTest, WriteMultipleFiles) {
  const std::string testId = "node1";
  const std::vector<std::string> testKeys = {"file1.txt, file2.txt, file3.txt"};

  // write several files with different content
  for (const auto &key : testKeys) {
    std::istringstream input("Data for " + key);
    auto bytesWritten = store->Write(testId, key, input);
    EXPECT_GT(bytesWritten, 0);
    EXPECT_TRUE(store->Has(testId, key));
  }

  // verify exach file exists and contains the correct data
  for (const auto &key : testKeys) {
    auto [size, stream] = store->Read(testId, key);
    EXPECT_GT(size, 0);
    ASSERT_TRUE(stream != nullptr);

    std::string readData;
    std::getline(*stream, readData);
    EXPECT_EQ(readData, "Data for " + key);
  }
}


/**
  Test handling of non-existent files.
  Verifies that:
  1. Has() returns false for non-existent files
  2. Read() returns appropriate null/zero values for non-existent files
  This tests the Store's error handling for missing files.
*/
TEST_F(StoreTest, NonExistentFile) {
  const std::string testId = "node1";
  const std::string testKey = "doesn't_exist.txt";

  // should return false for nonexistent files
  EXPECT_FALSE(store->Has(testId, testKey));

  // should return 0 size and null stream
  auto [size, stream] = store->Read(testId, testKey);
  EXPECT_EQ(size, 0);
  EXPECT_EQ(stream, nullptr);
}
