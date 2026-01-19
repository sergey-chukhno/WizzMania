#include "Packet.h"

#include <stdexcept>

// Platform-specific includes for ntohl/htonl
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace wizz {

// Constants
static const uint32_t MAGIC_NUMBER = 0xCAFEBABE;

Packet::Packet(PacketType type) {
  m_header.magic = MAGIC_NUMBER;
  m_header.type = static_cast<uint32_t>(type);
  m_header.length = 0;
  m_readOffset = 0;
}

Packet::Packet(const std::vector<uint8_t> &rawData) {
  // 1. Safety Check: Do we have at least a header?
  if (rawData.size() < sizeof(PacketHeader)) {
    throw std::runtime_error("Packet too small to contain header");
  }

  // 2. Extract Header
  const PacketHeader *rawHeader =
      reinterpret_cast<const PacketHeader *>(rawData.data());

  // 3. Convert from Network to Host Byte Order
  m_header.magic = ntohl(rawHeader->magic);
  m_header.type = ntohl(rawHeader->type);
  m_header.length = ntohl(rawHeader->length);

  // 4. Validate Magic
  if (m_header.magic != MAGIC_NUMBER) {
    throw std::runtime_error("Invalid Magic Number");
  }

  // 5. Extract Body (if any)
  // We expect the rest of the vector to be the body.
  // In a real socket loop, we would check if rawData.size() == sizeof(Header) +
  // m_header.length
  size_t headerSize = sizeof(PacketHeader);
  if (rawData.size() > headerSize) {
    m_body.assign(rawData.begin() + headerSize, rawData.end());
  }

  m_readOffset = 0;
}

void Packet::writeString(const std::string &str) {
  // 1. Write the length of the string first (so we know how much to read later)
  uint32_t len = static_cast<uint32_t>(str.size());
  writeInt(len);

  // 2. Write the characters
  writeData(str.data(), len);
}

void Packet::writeInt(uint32_t val) {
  // Network Byte Order: Big Endian
  uint32_t networkVal = htonl(val);
  writeData(&networkVal, sizeof(uint32_t));
}

void Packet::writeData(const void *data, size_t size) {
  const uint8_t *ptr = static_cast<const uint8_t *>(data);
  // Append data to the end of the body vector
  m_body.insert(m_body.end(), ptr, ptr + size);

  // Update the header length
  m_header.length += static_cast<uint32_t>(size);
}

std::vector<uint8_t> Packet::serialize() const {
  // 1. Prepare only the header first
  PacketHeader finalHeader = m_header;

  // 2. Convert Header fields to Network Byte Order
  finalHeader.magic = htonl(m_header.magic);
  finalHeader.type = htonl(m_header.type);
  finalHeader.length = htonl(m_header.length);

  // 3. Construct the full buffer
  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(PacketHeader) + m_body.size());

  // 4. Append Header
  const uint8_t *headerPtr = reinterpret_cast<const uint8_t *>(&finalHeader);
  buffer.insert(buffer.end(), headerPtr, headerPtr + sizeof(PacketHeader));

  // 5. Append Body
  buffer.insert(buffer.end(), m_body.begin(), m_body.end());

  return buffer;
}

uint32_t Packet::readInt() {
  // Bounds check
  if (m_readOffset + sizeof(uint32_t) > m_body.size()) {
    throw std::out_of_range("Not enough data to read uint32");
  }

  // Read 4 bytes
  uint32_t networkVal;
  std::memcpy(&networkVal, m_body.data() + m_readOffset, sizeof(uint32_t));

  // Advance cursor
  m_readOffset += sizeof(uint32_t);

  // Convert to Host Byte Order
  return ntohl(networkVal);
}

std::string Packet::readString() {
  // 1. Read Length
  uint32_t len = readInt();

  // 2. Bounds Check
  if (m_readOffset + len > m_body.size()) {
    throw std::out_of_range("Not enough data to read string");
  }

  // 3. Extract String
  std::string str(reinterpret_cast<const char *>(m_body.data() + m_readOffset),
                  len);

  // 4. Advance Cursor
  m_readOffset += len;

  return str;
}

} // namespace wizz
