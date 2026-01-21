#include "ClientSession.h"
#include <cstring> // for memcpy
#include <iostream>
#include <utility> // for std::move

// Needed for ntohl
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

// Include full definition for implementation
#include "DatabaseManager.h"

namespace wizz {

ClientSession::ClientSession(SocketType socket, DatabaseManager &db,
                             OnLoginCallback onLogin)
    : m_socket(socket), m_isLoggedIn(false), m_db(&db), m_onLogin(onLogin) {}

ClientSession::~ClientSession() {
  if (m_socket != INVALID_SOCKET_VAL) {
    close_socket_raw(m_socket);
  }
}

ClientSession::ClientSession(ClientSession &&other) noexcept
    : m_socket(other.m_socket), m_username(std::move(other.m_username)),
      m_isLoggedIn(other.m_isLoggedIn), m_db(other.m_db),
      m_onLogin(std::move(other.m_onLogin)),
      m_buffer(std::move(other.m_buffer)) {
  other.m_socket = INVALID_SOCKET_VAL;
}

// Move Assignment
ClientSession &ClientSession::operator=(ClientSession &&other) noexcept {
  if (this != &other) {
    if (m_socket != INVALID_SOCKET_VAL) {
      close_socket_raw(m_socket);
    }
    m_socket = other.m_socket;
    m_username = std::move(other.m_username);
    m_isLoggedIn = other.m_isLoggedIn;
    m_buffer = std::move(other.m_buffer);
    m_db = other.m_db; // Copy Pointer
    m_onLogin = std::move(other.m_onLogin);

    other.m_socket = INVALID_SOCKET_VAL;
  }
  return *this;
}

bool ClientSession::onDataReceived(const char *data, size_t length) {
  // 1. Append new data to the buffer
  m_buffer.insert(m_buffer.end(), data, data + length);

  // 2. Loop to process all complete packets
  while (true) {
    // A. Do we have a Header?
    if (m_buffer.size() < sizeof(PacketHeader)) {
      // Not enough data yet, wait for more
      break;
    }

    // B. Peek at the Length (to know how much to wait for)
    // Access raw bytes at offset 8 (Magic=0-3, Type=4-7, Length=8-11)
    uint32_t networkLength;
    std::memcpy(&networkLength, m_buffer.data() + 8, sizeof(uint32_t));
    uint32_t bodyLength = ntohl(networkLength);

    // Security: DoS Protection
    if (bodyLength > 10 * 1024 * 1024) { // 10MB limit
      std::cerr << "[Session] Error: Packet too large (" << bodyLength
                << " bytes). Disconnecting." << std::endl;
      return false; // Request disconnect
    }

    // C. Do we have the Full Packet? (Header + Body)
    size_t totalSize = sizeof(PacketHeader) + bodyLength;
    if (m_buffer.size() < totalSize) {
      // Not enough body yet, wait for more
      break;
    }

    // D. Extract and Process
    try {
      // Create a temporary vector for this packet
      std::vector<uint8_t> packetData(m_buffer.begin(),
                                      m_buffer.begin() + totalSize);

      Packet pkt(packetData); // Deserializes and validates Magic
      processPacket(pkt);

      // E. Remove processed bytes
      m_buffer.erase(m_buffer.begin(), m_buffer.begin() + totalSize);

    } catch (const std::exception &e) {
      std::cerr << "[Session] Packet Error: " << e.what() << std::endl;
      return false; // Malformed packet -> Disconnect
    }
  }

  return true; // Keep connection alive
}

void ClientSession::processPacket(Packet &packet) {
  // Dispatch based on Type
  switch (packet.type()) {
  case PacketType::Login:
    handleLogin(packet);
    break;

  case PacketType::Register:
    handleRegister(packet);
    break;

  default:
    std::cout << "[Session] Unknown Packet Type: "
              << static_cast<int>(packet.type()) << std::endl;
    break;
  }
}

void ClientSession::handleRegister(Packet &packet) {
  std::string username;
  std::string password;

  try {
    username = packet.readString();
    password = packet.readString();
  } catch (const std::exception &e) {
    std::cerr << "[Session] Register Protocol Error: " << e.what() << std::endl;
    return;
  }

  std::cout << "[Session] Register Attempt: " << username << std::endl;

  if (m_db && m_db->createUser(username, password)) {
    std::cout << "[Session] Registration SUCCESS for " << username << std::endl;
    // Auto-Login
    m_username = username;
    m_isLoggedIn = true;

    // Notify Server Registry
    if (m_onLogin) {
      m_onLogin(this);
    }

    Packet resp(PacketType::LoginSuccess); // Reuse LoginSuccess for now
    resp.writeString("Registration Successful! Welcome, " + username);

    std::vector<uint8_t> buffer = resp.serialize();
    send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
         0);
  } else {
    std::cout << "[Session] Registration FAILED (Taken): " << username
              << std::endl;
    Packet resp(PacketType::LoginFailed);
    resp.writeString("Username already taken.");

    std::vector<uint8_t> buffer = resp.serialize();
    send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
         0);
  }
}

void ClientSession::handleLogin(Packet &packet) {
  // 1. Parse Payload
  // Expected: [Username String] [Password String]
  std::string username;
  std::string password;

  try {
    username = packet.readString();
    password = packet.readString();
  } catch (const std::exception &e) {
    std::cerr << "[Session] Login Protocol Error: " << e.what() << std::endl;
    return; // Malformed packet
  }

  std::cout << "[Session] Login Attempt: " << username << std::endl;

  // 2. Verify with Database
  if (m_db && m_db->checkCredentials(username, password)) {
    std::cout << "[Session] Login SUCCESS for " << username << std::endl;
    m_username = username;
    m_isLoggedIn = true;

    // Notify Server Registry
    if (m_onLogin) {
      m_onLogin(this);
    }

    // 3. Send Response (LoginSuccess)
    Packet resp(PacketType::LoginSuccess);
    resp.writeString("Welcome to WizzMania, " + username + "!");

    std::vector<uint8_t> buffer = resp.serialize();
    send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
         0);

  } else {
    std::cout << "[Session] Login FAILED for " << username << std::endl;

    // 3. Send Response (LoginFailed)
    Packet resp(PacketType::LoginFailed);
    resp.writeString("Invalid Username or Password.");

    std::vector<uint8_t> buffer = resp.serialize();
    send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
         0);
  }
}

} // namespace wizz
