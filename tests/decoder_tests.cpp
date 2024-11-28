#include "network/decoder.h"
#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

namespace dfs {
namespace network {
namespace test {

class DecoderTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create acceptor for server side
    acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
        io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                    0 // Let system choose port
                                                    ));

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

    future.wait(); // Wait for connection to be established
    accept_thread.join();

    // Create decoder instance
    decoder_ = std::make_unique<BinaryDecoder>();
  }

  void TearDown() override {
    if (socket_ && socket_->is_open()) {
      socket_->close();
    }
    if (server_socket_ && server_socket_->is_open()) {
      server_socket_->close();
    }
    if (acceptor_ && acceptor_->is_open()) {
      acceptor_->close();
    }
  }

  // Helper method to write test data into the socket
  void WriteToSocket(const std::vector<uint8_t> &data) {
    boost::asio::write(*server_socket_, boost::asio::buffer(data));
  }

protected:
  boost::asio::io_context io_context_;
  std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
  std::unique_ptr<boost::asio::ip::tcp::socket> socket_;        // Client socket
  std::unique_ptr<boost::asio::ip::tcp::socket> server_socket_; // Server socket
  std::unique_ptr<BinaryDecoder> decoder_;
};

/**
 * Test: Stream Message Decoding
 * Purpose: Verify that the decoder correctly handles stream messages by:
 * 1. Reading the single-byte stream indicator
 * 2. Setting the stream flag in the RPC message
 * 3. Leaving the payload empty
 * 4. Returning correct number of bytes read
 */
TEST_F(DecoderTest, DecodeStreamMessage) {
  // Prepare stream message - just a type byte
  std::vector<uint8_t> data = {
      static_cast<uint8_t>(MessageType::IncomingStream)};

  // Write message to socket
  WriteToSocket(data);

  // Attempt to decode
  RPC msg;
  auto bytesRead = decoder_->Decode(*socket_, msg);

  // Verify decoded message properties
  EXPECT_EQ(bytesRead, 1);            // Should read 1 byte
  EXPECT_TRUE(msg.isStream());        // Stream flag should be set
  EXPECT_TRUE(msg.payload().empty()); // No payload for stream messages
}

/**
 * Test: Regular Message Decoding
 * Purpose: Verify that the decoder correctly handles regular messages by:
 * 1. Reading the message type byte
 * 2. Reading the 4-byte payload size
 * 3. Reading the complete payload
 * 4. Storing payload data correctly in the RPC message
 * 5. Setting appropriate stream flag status
 * 6. Returning correct total bytes read
 */
TEST_F(DecoderTest, DecodeRegularMessage) {
  // Create test message content
  std::string testData = "Hello, World!";
  uint32_t payloadSize = testData.size();

  // Build complete message
  std::vector<uint8_t> data;
  data.push_back(static_cast<uint8_t>(MessageType::IncomingMessage));

  // Add payload size in little endian format
  for (int i = 0; i < 4; i++) {
    data.push_back((payloadSize >> (i * 8)) & 0xFF);
  }

  // Add the actual payload data
  data.insert(data.end(), testData.begin(), testData.end());

  // Send message through socket
  WriteToSocket(data);

  // Decode the message
  RPC msg;
  auto bytesRead = decoder_->Decode(*socket_, msg);

  // Verify message was decoded correctly
  EXPECT_EQ(bytesRead, 1 + 4 + testData.size()); // type + size + payload
  EXPECT_FALSE(msg.isStream());                  // Not a stream message

  // Check payload contents match original
  std::string received(msg.payload().begin(), msg.payload().end());
  EXPECT_EQ(received, testData);
}

} // namespace test
} // namespace network
} // namespace dfs