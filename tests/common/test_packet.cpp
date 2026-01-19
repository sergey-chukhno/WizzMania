#include "../../common/Packet.h"
#include <cassert>
#include <iostream>
#include <vector>

// Simple manual test runner
void test_packet_serialization() {
  std::cout << "Running test_packet_serialization..." << std::endl;

  // 1. Create a Packet (as Sender)
  wizz::Packet packet(wizz::PacketType::Login);
  std::string username = "sergey";
  packet.writeString(username);
  packet.writeInt(42);

  // 2. Serialize to wire format
  std::vector<uint8_t> buffer = packet.serialize();

  // Verification 1: Check total size
  // Header (12) + StringLen (4) + "sergey" (6) + Int (4) = 26 bytes
  // Wait... 12 + 4 + 6 + 4 = 26.
  assert(buffer.size() == 26);

  // 3. Desserialize (as Receiver)
  // Simulate receiving raw bytes from network
  wizz::Packet receivedPacket(buffer);

  // Verification 2: Check Header
  assert(receivedPacket.type() == wizz::PacketType::Login);
  assert(receivedPacket.bodySize() == 14); // 4 + 6 + 4

  // Verification 3: Check Body Content
  std::string receivedName = receivedPacket.readString();
  uint32_t receivedInt = receivedPacket.readInt();

  assert(receivedName == "sergey");
  assert(receivedInt == 42);

  std::cout << "[PASS] test_packet_serialization" << std::endl;
}

void test_bounds_check() {
  std::cout << "Running test_bounds_check..." << std::endl;

  wizz::Packet p(wizz::PacketType::Error);
  // Write nothing
  std::vector<uint8_t> buffer = p.serialize();

  wizz::Packet recv(buffer);

  try {
    recv.readInt();
    assert(false && "Should have thrown out_of_range");
  } catch (const std::out_of_range &) {
    // Expected
  }

  std::cout << "[PASS] test_bounds_check" << std::endl;
}

int main() {
  try {
    test_packet_serialization();
    test_bounds_check();
    std::cout << "All tests passed!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Test Failed: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
