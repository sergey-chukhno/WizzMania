#pragma once

#include <cstdint>
#include <cstring> // for memcpy
#include <string>
#include <vector>

namespace wizz {

/**
 * @brief Enum representing the type of operation.
 * Used in the Packet Header to dispatch logic.
 */
enum class PacketType : uint32_t {
  // Auth
  Login = 100,
  Register = 101,
  LoginSuccess = 102,
  LoginFailed = 103,
  RegisterSuccess = 104,
  RegisterFailed = 105,

  // Contacts
  AddContact = 200,
  RemoveContact = 201,
  ContactList = 202,
  ContactStatusChange = 203,

  // Messaging
  DirectMessage = 300,
  MessageSent = 301,     // Acknowledge
  Nudge = 302,           // Wizz/Nudge
  VoiceMessage = 303,    // Voice Message (Binary Blob)
  TypingIndicator = 304, // Typing Status (Sender -> Server -> Target)

  // Errors

  // Errors
  Error = 999
};

/**
 * @brief The Fixed-Size Header prefixed to all transmissions.
 * Size: 12 Bytes (4 magic + 4 type + 4 length)
 */
struct PacketHeader {
  uint32_t magic;  // Should be 0xCAFEBABE
  uint32_t type;   // Cast to PacketType
  uint32_t length; // Length of the payload (excluding this header)
};

class Packet {
public:
  // Constructors
  explicit Packet(PacketType type);
  // Rehydrate from raw bytes (Receiver side)
  explicit Packet(const std::vector<uint8_t> &rawData);

  // Writing Data
  void writeString(const std::string &str);
  void writeInt(uint32_t val);
  void writeData(const void *data, size_t size);

  // Finalization
  // Returns the full byte buffer (Header + Body) ready for send()
  std::vector<uint8_t> serialize() const;

  // Reading Data (Stateful)
  uint32_t readInt();
  std::string readString();
  std::vector<uint8_t> readBytes(uint32_t len);

  // Accessors
  uint32_t bodySize() const { return static_cast<uint32_t>(m_body.size()); }
  PacketType type() const { return static_cast<PacketType>(m_header.type); }

private:
  PacketHeader m_header;
  std::vector<uint8_t> m_body;
  size_t m_readOffset = 0; // Cursor for reading
};

} // namespace wizz
