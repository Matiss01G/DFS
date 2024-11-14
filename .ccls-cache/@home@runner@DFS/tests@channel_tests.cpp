#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "network/channel.h"

using namespace dfs::network;

class ChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a new channel before each test
        channel = std::make_unique<Channel<int>>(2);  // Capacity of 2
    }

    std::unique_ptr<Channel<int>> channel;
};

/**
Test: Basic Operations
Purpose: Verify that the channel:
1. Can send and receive values correctly
2. Maintains proper empty/non-empty state
3. Preserves the values being sent
*/
TEST_F(ChannelTest, BasicOperations) {
    const int testValue = 42;
    channel->Send(testValue);

    EXPECT_FALSE(channel->Empty())
        << "Channel should not be empty after send";

    int received = channel->Receive();
    EXPECT_EQ(received, testValue)
        << "Received value should match sent value";

    EXPECT_TRUE(channel->Empty())
        << "Channel should be empty after receive";
}

/**
Test: Capacity Limits
Purpose: Verify that the channel:
1. Respects its capacity limit
2. Returns false for TrySend when full
3. Allows sending after space becomes available
*/
TEST_F(ChannelTest, CapacityLimits) {
    // Fill channel to capacity
    channel->Send(1);
    channel->Send(2);

    // Try to send when full
    bool sendResult = channel->TrySend(3);
    EXPECT_FALSE(sendResult)
        << "TrySend should fail when channel is full";

    // Receive one value
    EXPECT_EQ(channel->Receive(), 1)
        << "First value should be received correctly";

    // Now should be able to send again
    sendResult = channel->TrySend(3);
    EXPECT_TRUE(sendResult)
        << "TrySend should succeed after space is available";
}

/**
Test: Multi-threaded Operations
Purpose: Verify that the channel:
1. Maintains thread safety
2. Properly synchronizes between threads
3. Correctly handles concurrent operations
*/
TEST_F(ChannelTest, ThreadedOperations) {
    const int numValues = 100;
    std::vector<int> received;
    std::mutex receivedMutex;

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < numValues; i++) {
            channel->Send(i);
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < numValues; i++) {
            int value = channel->Receive();
            std::lock_guard<std::mutex> lock(receivedMutex);
            received.push_back(value);
        }
    });

    producer.join();
    consumer.join();

    // Verify we received all values
    EXPECT_EQ(received.size(), numValues)
        << "Should receive all sent values";

    // Sort received values (they might arrive in different order)
    std::sort(received.begin(), received.end());

    // Verify all values were received
    for (int i = 0; i < numValues; i++) {
        EXPECT_EQ(received[i], i)
            << "All values should be received exactly once";
    }
}

/**
Test: Non-blocking Operations
Purpose: Verify that TrySend and TryReceive:
1. Return immediately without blocking
2. Correctly indicate success/failure
3. Actually transfer values when successful
*/
TEST_F(ChannelTest, NonBlockingOperations) {
    // Test TryReceive on empty channel
    auto result = channel->TryReceive();
    EXPECT_FALSE(result.has_value())
        << "TryReceive should return empty optional on empty channel";

    // Test TrySend
    EXPECT_TRUE(channel->TrySend(42))
        << "TrySend should succeed on non-full channel";

    // Verify value was actually sent
    result = channel->TryReceive();
    EXPECT_TRUE(result.has_value())
        << "TryReceive should return value after successful TrySend";
    EXPECT_EQ(*result, 42)
        << "Received value should match sent value";
}