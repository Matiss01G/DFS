#include "network/decoder.h"
#include "network/tcp_transport.h"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace dfs::network;
using namespace std::chrono_literals;

class TCPTransportTest : public ::testing::Test {
protected:
  static uint16_t GetNextPort() {
    static std::atomic<uint16_t> port{3000};
    return port++;
  }

  void TearDown() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
};

TEST_F(TCPTransportTest, Construction) {
  TCPTransportOpts opts{.listenAddr = ":3000",
                        .handshakeFunc = NOPHandshakeFunc,
                        .decoder = std::make_shared<BinaryDecoder>()};

  auto transport = std::make_unique<TCPTransport>(opts);
  ASSERT_NE(transport, nullptr);
  EXPECT_EQ(transport->Addr(), ":3000");
}

TEST_F(TCPTransportTest, AddressHandling) {
  {
    auto port = GetNextPort();
    TCPTransportOpts opts{.listenAddr = ":" + std::to_string(port)};
    TCPTransport transport(opts);
    EXPECT_EQ(transport.Addr(), ":" + std::to_string(port));
  }

  {
    auto port = GetNextPort();
    TCPTransportOpts opts{.listenAddr = "127.0.0.1:" + std::to_string(port)};
    TCPTransport transport(opts);
    EXPECT_EQ(transport.Addr(), "127.0.0.1:" + std::to_string(port));
  }
}

TEST_F(TCPTransportTest, ListenAndAccept) {
  auto port = GetNextPort();
  TCPTransportOpts opts{.listenAddr = ":" + std::to_string(port),
                        .handshakeFunc = NOPHandshakeFunc,
                        .decoder = std::make_shared<BinaryDecoder>()};

  auto transport = std::make_unique<TCPTransport>(opts);
  ASSERT_TRUE(transport->ListenAndAccept());

  boost::asio::io_context io_context;
  std::vector<boost::asio::ip::tcp::socket> sockets;

  for (int i = 0; i < 3; i++) {
    sockets.emplace_back(io_context);
    sockets.back().connect(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), port));
  }

  transport->Close();
}

TEST_F(TCPTransportTest, DialOperation) {
  auto listener_port = GetNextPort();
  auto dialer_port = GetNextPort();

  TCPTransportOpts listener_opts{.listenAddr =
                                     ":" + std::to_string(listener_port),
                                 .handshakeFunc = NOPHandshakeFunc,
                                 .decoder = std::make_shared<BinaryDecoder>()};

  auto listener = std::make_unique<TCPTransport>(listener_opts);
  ASSERT_TRUE(listener->ListenAndAccept());

  TCPTransportOpts dialer_opts{.listenAddr = ":" + std::to_string(dialer_port),
                               .handshakeFunc = NOPHandshakeFunc,
                               .decoder = std::make_shared<BinaryDecoder>()};

  auto dialer = std::make_unique<TCPTransport>(dialer_opts);
  ASSERT_TRUE(dialer->Dial("127.0.0.1:" + std::to_string(listener_port)));

  dialer->Close();
  listener->Close();
}

TEST_F(TCPTransportTest, MessageConsumption) {
  auto port = GetNextPort();
  TCPTransportOpts opts{.listenAddr = ":" + std::to_string(port),
                        .handshakeFunc = NOPHandshakeFunc,
                        .decoder = std::make_shared<BinaryDecoder>()};

  auto transport = std::make_unique<TCPTransport>(opts);
  auto channel = transport->Consume();
  ASSERT_NE(channel, nullptr);

  std::thread producer([channel]() {
    RPC msg1{"peer1", {1, 2, 3}};
    RPC msg2{"peer2", {4, 5, 6}};
    channel->Send(std::move(msg1));
    channel->Send(std::move(msg2));
  });

  auto msg1 = channel->Receive();
  auto msg2 = channel->Receive();
  EXPECT_EQ(msg1.from(), "peer1");
  EXPECT_EQ(msg2.from(), "peer2");

  producer.join();
  transport->Close();
}

TEST_F(TCPTransportTest, HandshakeAndCallback) {
  auto port = GetNextPort();
  bool handshake_called = false;
  bool callback_called = false;

  TCPTransportOpts opts{
      .listenAddr = ":" + std::to_string(port),
      .handshakeFunc =
          [&](TCPPeer &peer) {
            handshake_called = true;
            return true;
          },
      .OnPeer = [&](TCPPeer &peer) { callback_called = true; },
      .decoder = std::make_shared<BinaryDecoder>()};

  // Create and start transport
  auto transport = std::make_unique<TCPTransport>(opts);
  ASSERT_TRUE(transport->ListenAndAccept());

  {
    // Socket in its own scope so it's destroyed before transport
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);

    // Connect but don't send anything
    socket.connect(boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), port));

    // Wait briefly for callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Socket will be closed at end of scope
  }

  // Verify callbacks before closing transport
  EXPECT_TRUE(handshake_called);
  EXPECT_TRUE(callback_called);

  transport->Close();
}