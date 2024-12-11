#include "crypto/crypto_utils.h"
#include "logging/logging.hpp"
#include "network/tcp_transport.h"
#include "server/file_server.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace dfs;
using namespace std::chrono_literals;

class FileServerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize logging once at test startup
    dfs::logging::init_logging("TEST");

    // create encryption key
    auto sharedKey = crypto::newEncryptionKey();

    // Setup transport options for server 1
    transportOpts1_.listenAddr = "0.0.0.0:58123";
    transportOpts1_.handshakeFunc = network::NOPHandshakeFunc;
    transportOpts1_.decoder = std::make_shared<network::BinaryDecoder>();

    // Setup transport options for server 2
    transportOpts2_.listenAddr = "0.0.0.0:58124";
    transportOpts2_.handshakeFunc = network::NOPHandshakeFunc;
    transportOpts2_.decoder = std::make_shared<network::BinaryDecoder>();

    // Create TCP transports
    transport1_ = std::make_shared<network::TCPTransport>(transportOpts1_);
    transport2_ = std::make_shared<network::TCPTransport>(transportOpts2_);

    // Setup server 1 options
    serverOpts1_.encKey = sharedKey;
    serverOpts1_.storageRoot = "test_storage_58123";
    serverOpts1_.pathTransformFunc = storage::CASPathTransformFunc;
    serverOpts1_.transport = transport1_;
    serverOpts1_.bootstrapNodes = {}; // No bootstrap nodes for first server

    // Setup server 2 options
    serverOpts2_.encKey = sharedKey;
    serverOpts2_.storageRoot = "test_storage_58124";
    serverOpts2_.pathTransformFunc = storage::CASPathTransformFunc;
    serverOpts2_.transport = transport2_;
    serverOpts2_.bootstrapNodes = {
        "127.0.0.1:58123"}; // Bootstrap from server 1
  }

  void TearDown() override {
    std::filesystem::remove_all(serverOpts1_.storageRoot);
    std::filesystem::remove_all(serverOpts2_.storageRoot);
    std::this_thread::sleep_for(100ms);
  }

protected:
  // Transport options
  network::TCPTransportOpts transportOpts1_;
  network::TCPTransportOpts transportOpts2_;

  // TCP transports
  std::shared_ptr<network::TCPTransport> transport1_;
  std::shared_ptr<network::TCPTransport> transport2_;

  // Server options
  server::FileServerOpts serverOpts1_;
  server::FileServerOpts serverOpts2_;
};

// TEST_F(FileServerTest, StartupAndShutdown) {
//   bool server1_success = false;
//   bool server2_success = false;
//   std::unique_ptr<server::FileServer> server1;
//   std::unique_ptr<server::FileServer> server2;

//   // Launch server1 in its own thread
//   std::thread server1_thread([&]() {
//     server1 = std::make_unique<server::FileServer>(serverOpts1_);
//     server1_success = server1->Start();
//   });

//   // Give time for first server to initialize
//   std::this_thread::sleep_for(500ms);
//   server1_thread.join();
//   ASSERT_TRUE(server1_success) << "Failed to start first server";

//   // Launch server2 in its own thread
//   std::thread server2_thread([&]() {
//     server2 = std::make_unique<server::FileServer>(serverOpts2_);
//     server2_success = server2->Start();
//   });

//   // Give time for second server to initialize
//   std::this_thread::sleep_for(500ms);
//   server2_thread.join();
//   ASSERT_TRUE(server2_success) << "Failed to start second server";

//   // Give servers time to run and potentially connect
//   std::this_thread::sleep_for(1s);

//   // Clean shutdown
//   if (server2)
//     server2->Stop();
//   if (server1)
//     server1->Stop();
// }

// TEST_F(FileServerTest, BasicStoreAndGet) {
//   // Start a single server
//   auto server = std::make_unique<server::FileServer>(serverOpts1_);
//   ASSERT_TRUE(server->Start());

//   // Give server time to initialize
//   std::this_thread::sleep_for(100ms);

//   // Test data
//   std::string testKey = "test_file.txt";
//   std::string testData = "Hello, this is test file content!";
//   std::stringstream dataStream(testData);

//   // Store the file
//   ASSERT_TRUE(server->Store(testKey, dataStream))
//       << "Failed to store test file";

//   // Retrieve the file
//   auto result = server->Get(testKey);
//   ASSERT_NE(result, nullptr) << "Failed to get test file";

//   // Read the content
//   std::stringstream resultStream;
//   resultStream << result->rdbuf();
//   std::string retrievedData = resultStream.str();

//   // Verify the content matches
//   EXPECT_EQ(retrievedData, testData)
//       << "Retrieved data doesn't match stored data";

//   // Test getting non-existent file
//   auto notFound = server->Get("nonexistent.txt");
//   EXPECT_EQ(notFound, nullptr) << "Expected null result for non-existent
//   file";

//   // Clean shutdown
//   server->Stop();
// }

// // Test 2: Verify two servers can connect properly
// TEST_F(FileServerTest, ServerConnectionTest) {
//   auto server1 = std::make_unique<server::FileServer>(serverOpts1_);
//   ASSERT_TRUE(server1->Start());
//   std::this_thread::sleep_for(500ms);

//   auto server2 = std::make_unique<server::FileServer>(serverOpts2_);
//   ASSERT_TRUE(server2->Start());
//   std::this_thread::sleep_for(1s);

//   server2->Stop();
//   server1->Stop();
// }

// // Test 3: Verify broadcast message mechanism works
// TEST_F(FileServerTest, BroadcastMessageTest) {
//   auto server1 = std::make_unique<server::FileServer>(serverOpts1_);
//   ASSERT_TRUE(server1->Start());
//   std::this_thread::sleep_for(500ms);

//   auto server2 = std::make_unique<server::FileServer>(serverOpts2_);
//   ASSERT_TRUE(server2->Start());
//   std::this_thread::sleep_for(1s);

//   // Store small file on server1
//   std::string testKey = "broadcast_test.txt";
//   std::string testData = "Small test data";
//   std::stringstream dataStream(testData);

//   // This should trigger broadcast - add logging to verify
//   ASSERT_TRUE(server1->Store(testKey, dataStream));

//   std::this_thread::sleep_for(1s);

//   server2->Stop();
//   server1->Stop();
// }

// TEST_F(FileServerTest, StreamTransferTest) {
//   auto server1 = std::make_unique<server::FileServer>(serverOpts1_);
//   auto server2 = std::make_unique<server::FileServer>(serverOpts2_);

//   ASSERT_TRUE(server1->Start());
//   std::this_thread::sleep_for(500ms);
//   ASSERT_TRUE(server2->Start());
//   std::this_thread::sleep_for(1s);

//   // Create test data that's big enough to ensure streaming
//   std::string testKey = "stream_test.txt";
//   std::stringstream dataStream;
//   for (int i = 0; i < 1000; i++) {
//     dataStream << "Test data block " << i << "\n";
//   }

//   ASSERT_TRUE(server1->Store(testKey, dataStream));
//   std::this_thread::sleep_for(2s);

//   server2->Stop();
//   server1->Stop();
// }

// Tests that when a file is stored on one server, it is automatically
// replicated to other connected servers
TEST_F(FileServerTest, StoreFileAcrossNetwork) {
  // Start both servers
  auto server1 = std::make_unique<server::FileServer>(serverOpts1_);
  ASSERT_TRUE(server1->Start());
  std::this_thread::sleep_for(500ms);

  auto server2 = std::make_unique<server::FileServer>(serverOpts2_);
  ASSERT_TRUE(server2->Start());

  // Give time for server2 to connect to server1 via bootstrap
  std::this_thread::sleep_for(3s);

  // Store file on server1
  std::string testKey = "distributed_test.txt";
  std::string testData = "Distributed storage test content";
  std::stringstream dataStream(testData);

  ASSERT_TRUE(server1->Store(testKey, dataStream))
      << "Failed to store test file on server1";

  std::this_thread::sleep_for(5s);

  // Try immediate retrieval from server2
  // This should work because the file should have been replicated
  auto result = server2->Get(testKey);
  ASSERT_NE(result, nullptr) << "Failed to get replicated file from server2";

  // Verify content
  std::stringstream resultStream;
  resultStream << result->rdbuf();
  EXPECT_EQ(resultStream.str(), testData)
      << "Retrieved data doesn't match original";

  server2->Stop();
  server1->Stop();
}

// // Tests that a server can fetch a file from another server when it doesn't
// // have it locally
// TEST_F(FileServerTest, FetchFileFromNetwork) {
//   // Start first server and store file
//   auto server1 = std::make_unique<server::FileServer>(serverOpts1_);
//   ASSERT_TRUE(server1->Start());

//   std::string testKey = "fetch_test.txt";
//   std::string testData = "Remote fetch test content";
//   std::stringstream dataStream(testData);

//   ASSERT_TRUE(server1->Store(testKey, dataStream))
//       << "Failed to store test file on server1";

//   // Now start server2 - after the file was stored
//   auto server2 = std::make_unique<server::FileServer>(serverOpts2_);
//   ASSERT_TRUE(server2->Start());

//   // Give time for server2 to connect to server1
//   std::this_thread::sleep_for(1s);

//   // Try to get file from server2
//   // This should trigger remote fetch since server2 started after storage
//   auto result = server2->Get(testKey);
//   ASSERT_NE(result, nullptr) << "Failed to fetch file from network";

//   // Verify content
//   std::stringstream resultStream;
//   resultStream << result->rdbuf();
//   EXPECT_EQ(resultStream.str(), testData)
//       << "Fetched data doesn't match original";

//   // Verify file is now cached locally on server2
//   auto cachedResult = server2->Get(testKey);
//   ASSERT_NE(cachedResult, nullptr) << "Failed to get cached file";

//   std::stringstream cachedStream;
//   cachedStream << cachedResult->rdbuf();
//   EXPECT_EQ(cachedStream.str(), testData)
//       << "Cached data doesn't match original";

//   server2->Stop();
//   server1->Stop();
// }