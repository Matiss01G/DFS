#include "network/decoder.h"
#include <boost/asio.hpp>
#include <gtest/gtest.h>

using namespace dfs::network;

class DecoderTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create acceptor for server side
    acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
        io_context_,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

    // Create client socket
    socket_ = std::make_unique<boost::asio::ip::tcp::socket>(io_context_);

    // Start accept operation
    std::promise<void> connected;
    auto future = connected.get_future();

    std::thread accept_thread([this, &connected]() {
      server_socket_ =
          std::make_unique<boost::asio::ip::tcp::socket>(io_context_);
      acceptor_->accept(*server_socket_);
      connected.set_value();
    });

    // Connect client socket to server
    socket_->connect(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::make_address("127.0.0.1"),
        acceptor_->local_endpoint().port()));

    future.wait();
    accept_thread.join();

    decoder_ = std::make_unique<BinaryDecoder>();
  }

  void WriteToSocket(const std::vector<uint8_t> &data) {
    boost::asio::write(*server_socket_, boost::asio::buffer(data));
  }

  // Helper to create regular message data
  std::vector<uint8_t> CreateMessageData(const std::string &payload) {
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(MessageType::IncomingMessage));

    uint32_t payloadSize = payload.size();
    uint32_t networkPayloadSize = htonl(payloadSize);
    const uint8_t *sizeBytes =
        reinterpret_cast<const uint8_t *>(&networkPayloadSize);
    data.insert(data.end(), sizeBytes, sizeBytes + sizeof(networkPayloadSize));

    data.insert(data.end(), payload.begin(), payload.end());
    return data;
  }

  void TearDown() override {
    if (socket_ && socket_->is_open())
      socket_->close();
    if (server_socket_ && server_socket_->is_open())
      server_socket_->close();
    if (acceptor_ && acceptor_->is_open())
      acceptor_->close();
  }

protected:
  boost::asio::io_context io_context_;
  std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
  std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
  std::unique_ptr<boost::asio::ip::tcp::socket> server_socket_;
  std::unique_ptr<BinaryDecoder> decoder_;
};

// Basic stream message test
// TEST_F(DecoderTest, DecodeStreamMessage) {
//   std::vector<uint8_t> data = {
//       static_cast<uint8_t>(MessageType::IncomingStream)};

//   WriteToSocket(data);

//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   EXPECT_EQ(bytesRead, 1);
//   EXPECT_TRUE(msg.isStream());
//   EXPECT_TRUE(msg.payload().empty());
// }

// // Basic regular message test
// TEST_F(DecoderTest, DecodeRegularMessage) {
//   std::string testData = "Hello, World!";
//   auto data = CreateMessageData(testData);

//   WriteToSocket(data);

//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   EXPECT_EQ(bytesRead, 1 + 4 + testData.size());
//   EXPECT_FALSE(msg.isStream());

//   std::string received(msg.payload().begin(), msg.payload().end());
//   EXPECT_EQ(received, testData);
// }

// // Test invalid message type
// TEST_F(DecoderTest, InvalidMessageType) {
//   std::vector<uint8_t> data = {0xFF}; // Invalid message type
//   WriteToSocket(data);

//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   EXPECT_EQ(bytesRead, -1);
// }

// // Test empty payload
// TEST_F(DecoderTest, EmptyPayload) {
//   auto data = CreateMessageData("");
//   WriteToSocket(data);

//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   EXPECT_EQ(bytesRead, 5); // 1 byte type + 4 bytes size + 0 bytes payload
//   EXPECT_TRUE(msg.payload().empty());
// }

// // Test maximum allowed payload
// TEST_F(DecoderTest, MaxAllowedPayload) {
//   std::string testData(1024 * 1024, 'A'); // 1MB of data
//   auto data = CreateMessageData(testData);

//   WriteToSocket(data);

//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   EXPECT_EQ(bytesRead, 1 + 4 + testData.size());
//   EXPECT_EQ(msg.payload().size(), testData.size());
// }

// // Test payload exceeding size limit
// TEST_F(DecoderTest, ExceedMaxPayload) {
//   // Create message with payload size larger than 1MB
//   std::vector<uint8_t> data;
//   data.push_back(static_cast<uint8_t>(MessageType::IncomingMessage));

//   uint32_t oversizedPayload = 1024 * 1024 + 1;
//   uint32_t networkSize = htonl(oversizedPayload);
//   const uint8_t *sizeBytes = reinterpret_cast<const uint8_t *>(&networkSize);
//   data.insert(data.end(), sizeBytes, sizeBytes + sizeof(networkSize));

//   WriteToSocket(data);

//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   EXPECT_EQ(bytesRead, -1);
// }

// Test partial message
TEST_F(DecoderTest, PartialMessage) {
  std::string testData = "Test";
  auto fullData = CreateMessageData(testData);

  // Only write part of the message
  std::vector<uint8_t> partialData(fullData.begin(), fullData.begin() + 3);
  WriteToSocket(partialData);

  RPC msg;
  auto bytesRead = decoder_->Decode(*socket_, msg);

  EXPECT_EQ(bytesRead, -1);
}

// // Test multiple messages in sequence
// TEST_F(DecoderTest, MultipleMessages) {
//   // Write several messages
//   std::vector<std::string> messages = {"First", "Second", "Third"};

//   for (const auto &message : messages) {
//     auto data = CreateMessageData(message);
//     WriteToSocket(data);

//     RPC msg;
//     auto bytesRead = decoder_->Decode(*socket_, msg);

//     EXPECT_EQ(bytesRead, 1 + 4 + message.size());
//     std::string received(msg.payload().begin(), msg.payload().end());
//     EXPECT_EQ(received, message);
//   }
// }

// #include "network/decoder.h"
// #include <boost/asio.hpp>
// #include <gtest/gtest.h>
// #include <memory>
// #include <thread>
// #include <vector>

// namespace dfs {
// namespace network {
// namespace test {

// class DecoderTest : public ::testing::Test {
// protected:
//   void SetUp() override {
//     // Create acceptor for server side
//     acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
//         io_context_,
//         boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
//                                                     0 // Let system choose
//                                                     port
//                                                     ));

//     // Create client socket
//     socket_ = std::make_unique<boost::asio::ip::tcp::socket>(io_context_);

//     // Start accept operation
//     std::promise<void> connected;
//     auto future = connected.get_future();

//     std::thread accept_thread([this, &connected]() {
//       server_socket_ =
//           std::make_unique<boost::asio::ip::tcp::socket>(io_context_);
//       acceptor_->accept(*server_socket_);
//       connected.set_value();
//     });

//     // Connect client socket to server
//     socket_->connect(boost::asio::ip::tcp::endpoint(
//         boost::asio::ip::make_address("127.0.0.1"),
//         acceptor_->local_endpoint().port()));

//     future.wait(); // Wait for connection to be established
//     accept_thread.join();

//     // Create decoder instance
//     decoder_ = std::make_unique<BinaryDecoder>();
//   }

//   void TearDown() override {
//     if (socket_ && socket_->is_open()) {
//       socket_->close();
//     }
//     if (server_socket_ && server_socket_->is_open()) {
//       server_socket_->close();
//     }
//     if (acceptor_ && acceptor_->is_open()) {
//       acceptor_->close();
//     }
//   }

//   // Helper method to write test data into the socket
//   void WriteToSocket(const std::vector<uint8_t> &data) {
//     boost::asio::write(*server_socket_, boost::asio::buffer(data));
//   }

// protected:
//   boost::asio::io_context io_context_;
//   std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
//   std::unique_ptr<boost::asio::ip::tcp::socket> socket_;        // Client
//   socket std::unique_ptr<boost::asio::ip::tcp::socket> server_socket_; //
//   Server socket std::unique_ptr<BinaryDecoder> decoder_;
// };

// /**
//  * Test: Stream Message Decoding
//  * Purpose: Verify that the decoder correctly handles stream messages by:
//  * 1. Reading the single-byte stream indicator
//  * 2. Setting the stream flag in the RPC message
//  * 3. Leaving the payload empty
//  * 4. Returning correct number of bytes read
//  */
// /**
//  * Test: Stream Message Decoding
//  * Purpose: Verify that the decoder correctly handles stream messages by:
//  * 1. Reading the single-byte stream indicator
//  * 2. Setting the stream flag in the RPC message
//  * 3. Leaving the payload empty
//  * 4. Returning correct number of bytes read
//  */
// TEST_F(DecoderTest, DecodeStreamMessage) {
//   // Prepare stream message - just a type byte
//   std::vector<uint8_t> data = {
//       static_cast<uint8_t>(MessageType::IncomingStream)};

//   // Write message to socket
//   WriteToSocket(data);

//   // Attempt to decode
//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   // Verify decoded message properties
//   EXPECT_EQ(bytesRead, 1);            // Should read 1 byte
//   EXPECT_TRUE(msg.isStream());        // Stream flag should be set
//   EXPECT_TRUE(msg.payload().empty()); // No payload for stream messages
// }

// /**
//  * Test: Regular Message Decoding
//  * Purpose: Verify that the decoder correctly handles regular messages by:
//  * 1. Reading the message type byte
//  * 2. Reading the 4-byte payload size
//  * 3. Reading the complete payload
//  * 4. Storing payload data correctly in the RPC message
//  * 5. Setting appropriate stream flag status
//  * 6. Returning correct total bytes read
//  */
// TEST_F(DecoderTest, DecodeRegularMessage) {
//   // Create test message content
//   std::string testData = "Hello, World!";
//   uint32_t payloadSize = testData.size();
//   uint32_t networkPayloadSize =
//       htonl(payloadSize); // Convert to network byte order

//   // Build complete message
//   std::vector<uint8_t> data;
//   data.push_back(static_cast<uint8_t>(MessageType::IncomingMessage));

//   // Add payload size in network byte order
//   const uint8_t *sizeBytes =
//       reinterpret_cast<const uint8_t *>(&networkPayloadSize);
//   data.insert(data.end(), sizeBytes, sizeBytes + sizeof(networkPayloadSize));

//   // Add the actual payload data
//   data.insert(data.end(), testData.begin(), testData.end());

//   // Send message through socket
//   WriteToSocket(data);

//   // Decode the message
//   RPC msg;
//   auto bytesRead = decoder_->Decode(*socket_, msg);

//   // Verify message was decoded correctly
//   EXPECT_EQ(bytesRead, 1 + 4 + testData.size()); // type + size + payload
//   EXPECT_FALSE(msg.isStream());                  // Not a stream message

//   // Check payload contents match original
//   std::string received(msg.payload().begin(), msg.payload().end());
//   EXPECT_EQ(received, testData);
// }

// } // namespace test
// } // namespace network
// } // namespace dfs