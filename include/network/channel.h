/**
* Thread-safe channel for passing messages between components.
* 
* This class provides a thread-safe way to pass messages between different
* parts of the distributed file system, similar to Go's channels. It ensures
* safe communication between:
* - Multiple sender threads
* - Multiple receiver threads
* - Transport layer and message handlers
* 
* The channel has a fixed capacity and will block senders when full and
* receivers when empty. It also provides non-blocking versions of send/receive
* operations for cases where blocking is not desired.
*/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace dfs {
namespace network {

template<typename T>
class Channel {
public:
    // Creates a channel with a max capacity to limit
    // number of messages that can be buffered
    explicit Channel(size_t capacity) 
        : capacity_(capacity) 
    {}

    // Sends a value through the channel
    // Blocks if the channel is full 
    void Send(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        // Wait until there's space in the queue
        not_full_.wait(lock, [this]() { 
            return queue_.size() < capacity_; 
        });

        queue_.push(std::move(value));
        not_empty_.notify_one();
    }

    // Attempts to send without blocking
    bool TrySend(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= capacity_) {
            return false;
        }

        queue_.push(std::move(value));
        not_empty_.notify_one();
        return true;
    }

    // Receives a value from the channel
    // Blocks if the channel is empty until a value arrives
    T Receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        // Wait until the queue has at least one element
        not_empty_.wait(lock, [this]() { 
            return !queue_.empty(); 
        });

        T value = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return value;
    }

    // Attempts to receive without blocking
    std::optional<T> TryReceive() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }z

        T value = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return value;
    }

    // Returns current number of items in channel
    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    // Checks if channel is empty
    bool Empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    const size_t capacity_;              // Max number of items in channel
    std::queue<T> queue_;                // Holds the queued messages
    mutable std::mutex mutex_;           // Protects access to queue
    std::condition_variable not_empty_;  // Signaled when queue becomes non-empty
    std::condition_variable not_full_;   // Signaled when space becomes available
};

} // namespace network
} // namespace dfs