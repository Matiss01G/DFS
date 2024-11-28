#include "network/message.h"
#include <gtest/gtest.h>
#include <sstream>

namespace dfs {
namespace network {
namespace testing {

/**
 * Test fixture for RPC message testing
 * Provides common test data for message addresses and payloads
 */
class MessageTest : public ::testing::Test {
protected:
  void SetUp() override {
    testFrom = "127.0.0.1:8080";                  // Sample network address
    testPayload = {0x01, 0x02, 0x03, 0x04, 0x05}; // Sample binary payload
  }
  std::string testFrom;
  std::vector<uint8_t> testPayload;
};

/**
Test: Basic Message Creation
Purpose: Verify that the RPC message:
1. Correctly stores the sender address
2. Properly maintains the payload data
3. Initializes stream flag to false by default
*/
TEST_F(MessageTest, CreateBasicMessage) {
  RPC msg(testFrom, testPayload);
  EXPECT_EQ(msg.from(), testFrom) << "Message should preserve sender address";
  EXPECT_EQ(msg.payload(), testPayload)
      << "Message should preserve payload data";
  EXPECT_FALSE(msg.isStream())
      << "Message should not be marked as stream by default";
}

/**
Test: Message Property Modification
Purpose: Verify that the RPC message:
1. Allows modification of all properties
2. Correctly stores modified values
3. Properly updates stream status
*/
TEST_F(MessageTest, SettersAndGetters) {
  RPC msg;
  msg.setFrom(testFrom);
  msg.setPayload(testPayload);
  msg.setStream(true);

  EXPECT_EQ(msg.from(), testFrom) << "Setter should update sender address";
  EXPECT_EQ(msg.payload(), testPayload) << "Setter should update payload data";
  EXPECT_TRUE(msg.isStream()) << "Setter should update stream status";
}

/**
Test: Message Serialization
Purpose: Verify that the RPC message:
1. Can be serialized to a binary stream
2. Can be deserialized from a binary stream
3. Maintains all properties through serialization
4. Preserves data integrity in the process
*/
TEST_F(MessageTest, SerializeDeserialize) {
  // Create and serialize original message
  RPC original(testFrom, testPayload, true);
  std::stringstream buffer;
  ASSERT_TRUE(original.serialize(buffer))
      << "Message serialization should succeed";

  // Deserialize into new message
  RPC deserialized;
  ASSERT_TRUE(deserialized.deserialize(buffer))
      << "Message deserialization should succeed";

  // Verify all properties match
  EXPECT_EQ(deserialized.from(), original.from())
      << "Sender address should survive serialization";
  EXPECT_EQ(deserialized.payload(), original.payload())
      << "Payload should survive serialization";
  EXPECT_EQ(deserialized.isStream(), original.isStream())
      << "Stream status should survive serialization";
}

} // namespace testing
} // namespace network
} // namespace dfs