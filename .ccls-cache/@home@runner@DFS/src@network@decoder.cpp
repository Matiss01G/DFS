#include "network/decoder.h"
#include <iostream>

namespace dfs {
namespace network {

ssize_t BinaryDecoder::Decode(boost::asio::ip::tcp::socket& socket, RPC& msg) {
    try {
        // First byte indicates message type:
        // - IncomingMessage (0x1): Regular message with payload
        // - IncomingStream (0x2): Stream message with no immediate payload        
        uint8_t msgType;
        if (!ReadExact(socket, &msgType, 1)) {
            return -1;
        }

        // If this is a stream message, we don't read any payload
        if (msgType == static_cast<uint8_t>(MessageType::IncomingStream)) {
            msg.setStream(true);
            return 1; // Return bytes read
        }

        // For regular messages, next 4 bytes contain the payload size
        uint32_t payloadSize;
        if (!ReadExact(socket, reinterpret_cast<uint8_t*>(&payloadSize), 
                      sizeof(payloadSize))) {
            return -1;
        }

        // Protect against malicious or corrupted messages by limiting payload size
        if (payloadSize > 1024 * 1024) { // 1MB limit
            std::cerr << "Payload size too large: " << payloadSize << std::endl;
            return -1;
        }

        // Allocate buffer and read the actual payload data
        std::vector<uint8_t> payload(payloadSize);
        if (!ReadExact(socket, payload.data(), payloadSize)) {
            return -1;
        }

        msg.setStream(false);
        msg.setPayload(std::move(payload));

        // Return total bytes read
        return 1 + sizeof(payloadSize) + payloadSize;

    } catch (const boost::system::system_error& e) {
        std::cerr << "Decode error: " << e.what() << std::endl;
        return -1;
    }
}

// Helper method to ensure we read exactly the requested number of bytes
bool BinaryDecoder::ReadExact(boost::asio::ip::tcp::socket& socket, 
                             uint8_t* buffer, 
                             size_t len) {
    size_t totalRead = 0;

    // continue reading until we have all requested bytes
    while (totalRead < len) {
        boost::system::error_code ec;

        // Read as many bytes as possible into the remaining buffer space
        size_t bytesRead = socket.read_some(
            boost::asio::buffer(buffer + totalRead, len - totalRead), 
            ec
        );

        if (ec) {
            std::cerr << "Read error: " << ec.message() << std::endl;
            return false;
        }

        // Check for connection closure
        if (bytesRead == 0) {
            std::cerr << "Connection closed by peer" << std::endl;
            return false;
        }

        totalRead += bytesRead;
    }
    return true;
}

} // namespace network
} // namespace dfs