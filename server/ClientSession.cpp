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
                             OnLoginCallback onLogin,
                             OnMessageCallback onMessage,
                             OnNudgeCallback onNudge,
                             OnVoiceMessageCallback onVoiceMessage,
                             OnTypingIndicatorCallback onTypingIndicator,
                             GetStatusCallback getStatus,
                             OnStatusChangeCallback onStatusChange)
    : m_socket(socket), m_isLoggedIn(false), m_db(&db), m_onLogin(onLogin),
      m_onMessage(onMessage), m_onNudge(onNudge),
      m_onVoiceMessage(onVoiceMessage), m_onTypingIndicator(onTypingIndicator),
      m_getStatus(getStatus), m_onStatusChange(onStatusChange) {}

ClientSession::~ClientSession() {
  if (m_socket != INVALID_SOCKET_VAL) {
    close_socket_raw(m_socket);
  }
}

ClientSession::ClientSession(ClientSession &&other) noexcept
    : m_socket(other.m_socket), m_username(std::move(other.m_username)),
      m_isLoggedIn(other.m_isLoggedIn), m_db(other.m_db),
      m_onLogin(std::move(other.m_onLogin)),
      m_onMessage(std::move(other.m_onMessage)),
      m_onNudge(std::move(other.m_onNudge)),
      m_onVoiceMessage(std::move(other.m_onVoiceMessage)),
      m_onTypingIndicator(std::move(other.m_onTypingIndicator)),
      m_getStatus(std::move(other.m_getStatus)),
      m_onStatusChange(std::move(other.m_onStatusChange)),
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
    m_onMessage = std::move(other.m_onMessage);
    m_onNudge = std::move(other.m_onNudge);
    m_onVoiceMessage = std::move(other.m_onVoiceMessage);
    m_onTypingIndicator = std::move(other.m_onTypingIndicator);
    m_getStatus = std::move(other.m_getStatus);
    m_onStatusChange = std::move(other.m_onStatusChange);

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

  case PacketType::DirectMessage:
    handleDirectMessage(packet);
    break;

  case PacketType::AddContact:
    handleAddContact(packet);
    break;

  case PacketType::RemoveContact:
    handleRemoveContact(packet);
    break;

  case PacketType::ContactStatusChange:
    handleStatusChange(packet);
    break;

  case PacketType::Nudge:
    handleNudge(packet);
    break;

  case PacketType::VoiceMessage:
    handleVoiceMessage(packet);
    break;

  case PacketType::TypingIndicator:
    // Inline handling for simplicity as it's small
    {
      if (!m_isLoggedIn)
        break;
      std::string targetUser = packet.readString();
      // We expect an Int (0 or 1) for visibility
      // Read as Int, cast to bool
      bool isTyping = (packet.readInt() != 0);

      if (m_onTypingIndicator) {
        m_onTypingIndicator(this, targetUser, isTyping);
      }
    }
    break;

  default:
    std::cout << "[Session] Unknown Packet Type: "
              << static_cast<int>(packet.type()) << std::endl;
    break;
  }
}

void ClientSession::sendPacket(const Packet &packet) {
  std::vector<uint8_t> buffer = packet.serialize();
  send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
       0);
}

void ClientSession::handleDirectMessage(Packet &packet) {
  if (!m_isLoggedIn)
    return; // Ignore if not logged in

  std::string targetUser;
  std::string messageBody;
  try {
    targetUser = packet.readString();
    messageBody = packet.readString();
  } catch (const std::exception &e) {
    std::cerr << "[Session] Message Protocol Error: " << e.what() << std::endl;
    return;
  }

  std::cout << "[Session] Logic: " << m_username << " -> " << targetUser << ": "
            << messageBody << std::endl;

  // Trigger Callback to Router
  if (m_onMessage) {
    m_onMessage(this, targetUser, messageBody);
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

    // 1. Send Success Packet FIRST
    Packet resp(PacketType::RegisterSuccess);
    resp.writeString("Registration Successful! Welcome, " + username);

    std::vector<uint8_t> buffer = resp.serialize();
    send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
         0);

    // 2. Notify Server Registry (which might flush Offline Msgs)
    if (m_onLogin) {
      m_onLogin(this);
    }
  } else {
    std::cout << "[Session] Registration FAILED (Taken): " << username
              << std::endl;
    Packet resp(PacketType::RegisterFailed);
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

    // 3. Send Response (LoginSuccess) FIRST
    Packet resp(PacketType::LoginSuccess);
    resp.writeString("Welcome to WizzMania, " + username + "!");

    std::vector<uint8_t> buffer = resp.serialize();
    send(m_socket, reinterpret_cast<const char *>(buffer.data()), buffer.size(),
         0);

    // 4. Send Contact List (Sync)
    std::vector<std::string> friends = m_db->getFriends(username);
    if (!friends.empty()) {
      Packet contactList(PacketType::ContactList);
      contactList.writeInt(static_cast<uint32_t>(friends.size()));
      for (const auto &name : friends) {
        contactList.writeString(name);
        // Sync Status
        int status = m_getStatus ? m_getStatus(name) : 3; // 3 = Offline default
        contactList.writeInt(static_cast<uint32_t>(status));
      }
      sendPacket(contactList);
    }

    // 5. Notify Server Registry (Triggers Offline Msg Flush)
    if (m_onLogin) {
      m_onLogin(this);
    }

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

void ClientSession::handleAddContact(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  std::string targetUser;
  try {
    targetUser = packet.readString();
  } catch (...) {
    return;
  }

  std::cout << "[Session] Add Contact: " << m_username << " -> " << targetUser
            << std::endl;

  if (m_db && m_db->addFriend(m_username, targetUser)) {
    // Success: Send Updated List (Sync)
    std::vector<std::string> friends = m_db->getFriends(m_username);
    Packet resp(PacketType::ContactList);
    resp.writeInt(static_cast<uint32_t>(friends.size()));
    for (const auto &name : friends) {
      resp.writeString(name);
      int status = m_getStatus ? m_getStatus(name) : 3;
      resp.writeInt(static_cast<uint32_t>(status));
    }
    sendPacket(resp);
  } else {
    // Fail: User not found
    Packet err(PacketType::Error);
    err.writeString("Failed to add contact: User not found.");
    sendPacket(err);
  }
}

void ClientSession::handleRemoveContact(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  std::string targetUser;
  try {
    targetUser = packet.readString();
  } catch (...) {
    return;
  }

  std::cout << "[Session] Remove Contact: " << m_username << " -> "
            << targetUser << std::endl;

  if (m_db && m_db->removeFriend(m_username, targetUser)) {
    // Send Updated List
    std::vector<std::string> friends = m_db->getFriends(m_username);
    Packet resp(PacketType::ContactList);
    resp.writeInt(static_cast<uint32_t>(friends.size()));
    for (const auto &name : friends) {
      resp.writeString(name);
      int status = m_getStatus ? m_getStatus(name) : 3;
      resp.writeInt(static_cast<uint32_t>(status));
    }
    sendPacket(resp);
  }
}

void ClientSession::handleStatusChange(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  int newStatus;
  try {
    newStatus = static_cast<int>(packet.readInt());
    // Optional status message reading if protocol supports it
    // std::string msg = packet.readString();
  } catch (...) {
    return;
  }

  std::cout << "[Session] Status Change: " << m_username << " -> " << newStatus
            << std::endl;

  if (m_onStatusChange) {
    m_onStatusChange(this, newStatus);
  }
}

void ClientSession::handleNudge(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  std::string targetUser;
  try {
    targetUser = packet.readString();
  } catch (...) {
    return;
  }

  std::cout << "[Session] Nudge: " << m_username << " -> " << targetUser
            << std::endl;

  if (m_onNudge) {
    m_onNudge(this, targetUser);
  }
}

void ClientSession::handleVoiceMessage(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  std::string targetUser;
  uint16_t duration = 0;
  uint32_t dataLen = 0;
  std::vector<uint8_t> audioData;

  try {
    targetUser = packet.readString();
    duration = static_cast<uint16_t>(packet.readInt());
    dataLen = packet.readInt();

    // Safety check on dataLen
    if (dataLen > 10 * 1024 * 1024) { // 10MB sanity cap
      std::cerr << "[Session] Voice Message too large: " << dataLen
                << std::endl;
      return;
    }
    audioData = packet.readBytes(dataLen);

  } catch (const std::exception &e) {
    std::cerr << "[Session] Voice Protocol Error: " << e.what() << std::endl;
    return;
  }

  std::cout << "[Session] Voice Message: " << m_username << " -> " << targetUser
            << " (" << duration << "s, " << dataLen << " bytes)" << std::endl;

  if (m_onVoiceMessage) {
    m_onVoiceMessage(this, targetUser, duration, audioData);
  }
}

} // namespace wizz
