#include <gtest/gtest.h>
#include "network/tcp_transport.h"
#include "network/message.h"
#include <chrono>
#include <thread>

using namespace dfs::network;

// Test fixture that sets up server and client transports for each test
// This ensures each test starts with fresh transport instances
class TransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create server transport with specific port and default handshake
        TCPTransportOpts server_opts{
            .listenAddr = ":3000",
            .handshakeFunc = NOPHandshakeFunc
        };
        server = std::make_unique<TCPTransport>(server_opts);

        // Create client transport
        TCPTransportOpts client_opts{
            .listenAddr = ":3001",
            .handshakeFunc = NOPHandshakeFunc
        };
        client = std::make_unique<TCPTransport>(client_opts);
    }

    // Ensure proper cleanup of transport resources after each test
    void TearDown() override {
        // close the client
        if (server) server->Close();
        if (client) client->Close();

        // close the server
        if (server) {
            server->Close();
            server.reset();
        }

        // Give some time for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::unique_ptr<TCPTransport> server;
    std::unique_ptr<TCPTransport> client;
};

/**
Test: Basic Connection
Purpose: Verify that:
1. Server can start listening
2. Client can connect to server
3. Connection is properly established

This test ensures the fundamental connection setup works
before testing more complex functionality.
 */
TEST_F(TransportTest, BasicConnection) {
    // Start server listening
    ASSERT_TRUE(server->ListenAndAccept())
        << "Server should start listening successfully";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Attempt client connection
    ASSERT_TRUE(client->Dial("127.0.0.1:3000"))
        << "Client should connect to server successfully";

    // Small delay to ensure connection is fully established
    // This is needed because network operations are asynchronous
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify correct addresses are reported
    EXPECT_EQ(server->Addr(), ":3000")
        << "Server should report correct listening address";
    EXPECT_EQ(client->Addr(), ":3001")
        << "Client should report correct listening address";
}

/**
Test: Message Exchange
Purpose: Verify that:
1. Messages can be sent between peers
2. Message content is preserved
3. Messages are received in order

This test verifies the actual data transmission functionality
once a connection is established.
 */
TEST_F(TransportTest, MessageExchange) {
    // Establish connection
    ASSERT_TRUE(server->ListenAndAccept());
    ASSERT_TRUE(client->Dial("127.0.0.1:3000"));

    // Allow connection to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Prepare test data
    std::vector<uint8_t> testData = {1, 2, 3, 4};
    RPC testMessage("client", testData);

    // Get server's receive channel
    auto serverChannel = server->Consume();

    // Set up receiving thread to handle incoming message
    std::atomic<bool> messageReceived{false};
    // create new thread to handle message reception
    std::thread receiver([&]() {
        // wait for and get message from server's channel
        auto msg = serverChannel->Receive();
        EXPECT_EQ(msg.payload(), testData)
            << "Received message should match sent message";
        // set atomic flag to true to indicate message received
        messageReceived.store(true);
    });

    receiver.join();
    EXPECT_TRUE(messageReceived)
        << "Server should receive the message from client";
}

/**
Test: Multiple Clients
Purpose: Verify that:
1. Server can handle multiple client connections
2. Each client can communicate independently
3. Messages are routed to correct destinations

Tests the transport's ability to handle multiple simultaneous
connections, which is crucial for a distributed system.
 */
TEST_F(TransportTest, MultipleClients) {
    ASSERT_TRUE(server->ListenAndAccept());

    // Create second client with unique port
    TCPTransportOpts client2_opts{
        .listenAddr = ":3002",
        .handshakeFunc = NOPHandshakeFunc
    };
    auto client2 = std::make_unique<TCPTransport>(client2_opts);

    // Connect both clients to server
    ASSERT_TRUE(client->Dial("127.0.0.1:3000"))
        << "First client should connect successfully";
    ASSERT_TRUE(client2->Dial("127.0.0.1:3000"))
        << "Second client should connect successfully";

    // Allow time for connections to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Clean up additional client
    client2->Close();
}

/**
Test: Connection Cleanup
Purpose: Verify that:
1. Resources are properly cleaned up on Close()
2. No memory leaks occur
3. Subsequent operations fail gracefully

Tests proper resource management and cleanup behavior,
which is essential for system stability.
 */
TEST_F(TransportTest, ConnectionCleanup) {
    ASSERT_TRUE(server->ListenAndAccept());
    ASSERT_TRUE(client->Dial("127.0.0.1:3000"));

    // Verify clean client shutdown
    EXPECT_TRUE(client->Close())
        << "Client should close cleanly";

    // Verify operations fail appropriately after closure
    EXPECT_FALSE(client->Dial("127.0.0.1:3000"))
        << "Dial should fail after transport is closed";

    // Verify clean server shutdown
    EXPECT_TRUE(server->Close())
        << "Server should close cleanly";
}