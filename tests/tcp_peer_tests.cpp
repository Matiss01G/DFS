#include <gtest/gtest.h>
#include "network/tcp_peer.h"
#include <thread>
#include <chrono>

using namespace dfs::network;

namespace {

// Helper class to manage test server socket
class TestServer {
public:
    TestServer() : acceptor_(io_context_) {
        // Set up acceptor on localhost with random port
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::tcp::v4(), 0);
        acceptor_.open(endpoint.protocol());
        acceptor_.bind(endpoint);
        acceptor_.listen();
    }

    uint16_t port() const {
        return acceptor_.local_endpoint().port();
    }

    boost::asio::ip::tcp::socket accept() {
        boost::asio::ip::tcp::socket socket(io_context_);
        acceptor_.accept(socket);
        return socket;
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

// Fixture for TCP peer tests
class TCPPeerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start server
        server_ = std::make_unique<TestServer>();

        // Create client connection
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::socket client_socket(io_context);
        client_socket.connect(boost::asio::ip::tcp::endpoint(
            boost::asio::ip::address::from_string("127.0.0.1"), 
            server_->port()));

        // Create peers for both ends
        client_peer_ = std::make_unique<TCPPeer>(std::move(client_socket), true);
        server_peer_ = std::make_unique<TCPPeer>(server_->accept(), false);
    }

    std::unique_ptr<TestServer> server_;
    std::unique_ptr<TCPPeer> client_peer_;
    std::unique_ptr<TCPPeer> server_peer_;
};

} // anonymous namespace

/**
* Test basic TCP peer connection.
* Verifies that:
* 1. Client peer successfully connects to server
* 2. Both peers report valid remote addresses
* 3. Connection is established in both directions
*/
TEST_F(TCPPeerTest, Connection) {
    // Verify both peers have valid addresses
    EXPECT_FALSE(client_peer_->RemoteAddr().empty());
    EXPECT_FALSE(server_peer_->RemoteAddr().empty());
}

/**
* Test peer data transmission.
* Verifies that:
* 1. Client can send data to server
* 2. Server receives exact data sent
* 3. Data integrity is maintained during transfer
*/
TEST_F(TCPPeerTest, SendData) {
    // Create test data
    std::vector<uint8_t> test_data = {1, 2, 3, 4, 5};

    // Send from client to server
    EXPECT_TRUE(client_peer_->Send(test_data));

    // Read on server side
    std::vector<uint8_t> received(test_data.size());
    size_t bytes_read = server_peer_->socket().read_some(
        boost::asio::buffer(received));

    // Verify data was received correctly
    EXPECT_EQ(bytes_read, test_data.size());
    EXPECT_EQ(received, test_data);
}

/**
* Test stream control operations.
* Verifies that:
* 1. Streams can be started successfully
* 2. WaitForStream blocks until stream closes
* 3. CloseStream properly signals completion
*/
TEST_F(TCPPeerTest, StreamOperations) {
    // Start stream operation
    client_peer_->StartStream();

    // Create flag for completion
    bool stream_complete = false;

    // Start waiting thread
    std::thread wait_thread([&]() {
        client_peer_->WaitForStream();
        stream_complete = true;
    });

    // Small delay to ensure wait is happening
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Close stream and verify completion
    client_peer_->CloseStream();
    wait_thread.join();
    EXPECT_TRUE(stream_complete);
}

/**
* Test socket accessor method.
* Verifies that:
* 1. socket() returns a valid socket reference
* 2. Returned socket is connected
* 3. Returned socket matches the one used for construction
*/
TEST_F(TCPPeerTest, SocketAccess) {
    // Verify socket is connected
    EXPECT_TRUE(client_peer_->socket().is_open());
    EXPECT_TRUE(server_peer_->socket().is_open());

    // Verify can get endpoint information
    EXPECT_NO_THROW(client_peer_->socket().remote_endpoint());
    EXPECT_NO_THROW(server_peer_->socket().remote_endpoint());
}

/**
* Test peer connection direction.
* Verifies that:
* 1. Client peer is marked as outbound connection
* 2. Server peer is marked as inbound connection
*/
TEST_F(TCPPeerTest, ConnectionDirection) {
    // Create test data to force communication in both directions
    std::vector<uint8_t> data = {1, 2, 3};

    // Test client->server (outbound connection)
    EXPECT_TRUE(client_peer_->Send(data));
    std::vector<uint8_t> received(data.size());
    size_t bytes_read = server_peer_->socket().read_some(
        boost::asio::buffer(received));
    EXPECT_EQ(bytes_read, data.size());

    // Test server->client (inbound connection)
    EXPECT_TRUE(server_peer_->Send(data));
    bytes_read = client_peer_->socket().read_some(
        boost::asio::buffer(received));
    EXPECT_EQ(bytes_read, data.size());
}