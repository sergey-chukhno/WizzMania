#include <gtest/gtest.h>
#include "../../common/Packet.h"
#include <vector>
#include <string>

/**
 * @brief Unit tests for the wizz::Packet protocol.
 * Verifies serialization, deserialization, and boundary safety.
 */

TEST(PacketTest, SerializationRoundTrip) {
    // 1. Create a Packet
    wizz::Packet packet(wizz::PacketType::Login);
    std::string username = "sergey";
    uint32_t secretId = 42;
    
    packet.writeString(username);
    packet.writeInt(secretId);

    // 2. Serialize
    std::vector<uint8_t> buffer = packet.serialize();

    // Verification: Header (12) + StringLen (4) + "sergey" (6) + Int (4) = 26 bytes
    EXPECT_EQ(buffer.size(), 26);

    // 3. Deserialize
    wizz::Packet received(buffer);

    EXPECT_EQ(received.type(), wizz::PacketType::Login);
    EXPECT_EQ(received.bodySize(), 14); // 4 + 6 + 4

    // 4. Read back content
    EXPECT_EQ(received.readString(), "sergey");
    EXPECT_EQ(received.readInt(), 42);
}

TEST(PacketTest, BoundsCheck) {
    wizz::Packet p(wizz::PacketType::Error);
    std::vector<uint8_t> buffer = p.serialize();

    wizz::Packet recv(buffer);

    // Attempt to read data that doesn't exist
    EXPECT_THROW(recv.readInt(), std::out_of_range);
    EXPECT_THROW(recv.readString(), std::out_of_range);
}

TEST(PacketTest, MultipleStringsAndInts) {
    wizz::Packet p(wizz::PacketType::DirectMessage);
    p.writeString("Sender");
    p.writeString("Recipient");
    p.writeInt(100);
    p.writeString("Message Body");
    p.writeInt(200);

    wizz::Packet recv(p.serialize());

    EXPECT_EQ(recv.readString(), "Sender");
    EXPECT_EQ(recv.readString(), "Recipient");
    EXPECT_EQ(recv.readInt(), 100);
    EXPECT_EQ(recv.readString(), "Message Body");
    EXPECT_EQ(recv.readInt(), 200);
}

TEST(PacketTest, EmptyStrings) {
    wizz::Packet p(wizz::PacketType::UpdateStatus);
    p.writeString("");
    p.writeString("not empty");
    p.writeString("");

    wizz::Packet recv(p.serialize());

    EXPECT_EQ(recv.readString(), "");
    EXPECT_EQ(recv.readString(), "not empty");
    EXPECT_EQ(recv.readString(), "");
}

TEST(PacketTest, LargeInts) {
    wizz::Packet p(wizz::PacketType::GameStatus);
    p.writeInt(0xFFFFFFFF);
    p.writeInt(0);
    p.writeInt(12345678);

    wizz::Packet recv(p.serialize());

    EXPECT_EQ(recv.readInt(), 0xFFFFFFFF);
    EXPECT_EQ(recv.readInt(), 0);
    EXPECT_EQ(recv.readInt(), 12345678);
}
