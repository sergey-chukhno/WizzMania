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
#include "TcpServer.h"

namespace wizz {

ClientSession::ClientSession(
    int sessionId, asio::ip::tcp::socket socket, TcpServer *server,
    OnLoginCallback onLogin, OnMessageCallback onMessage,
    OnNudgeCallback onNudge, OnVoiceMessageCallback onVoiceMessage,
    OnTypingIndicatorCallback onTypingIndicator, GetStatusCallback getStatus,
    OnStatusChangeCallback onStatusChange,
    OnUpdateAvatarCallback onUpdateAvatar, OnGetAvatarCallback onGetAvatar)
    : m_sessionId(sessionId), m_socket(std::move(socket)), m_isLoggedIn(false),
      m_server(server), m_onLogin(onLogin), m_onMessage(onMessage),
      m_onNudge(onNudge), m_onVoiceMessage(onVoiceMessage),
      m_onTypingIndicator(onTypingIndicator), m_getStatus(getStatus),
      m_onStatusChange(onStatusChange), m_onUpdateAvatar(onUpdateAvatar),
      m_onGetAvatar(onGetAvatar) {}

ClientSession::~ClientSession() {
  if (m_socket.is_open()) {
    asio::error_code ec;
    m_socket.close(ec);
  }
}

void ClientSession::sendPacket(const Packet &packet) {
  // Serialize Packet
  std::vector<uint8_t> data = packet.serialize();

  // To avoid lifetime issues where 'data' goes out of scope before the async
  // execution, we must heap-allocate the buffer and capture it in the lambda.
  auto buffer = std::make_shared<std::vector<uint8_t>>(std::move(data));
  auto self(shared_from_this());

  asio::async_write(
      m_socket, asio::buffer(*buffer),
      [this, self, buffer](asio::error_code ec, std::size_t /*length*/) {
        if (ec) {
          std::cerr << "[Session " << m_sessionId
                    << "] Write Error: " << ec.message() << std::endl;
        }
      });
}

void ClientSession::start() { doRead(); }

void ClientSession::doRead() {
  auto self(shared_from_this());

  // We read enough for the packet header first.
  // Instead of a loop, we rely on callbacks holding a shared_ptr to keep the
  // Session alive.
  m_buffer.resize(1024); // Generic read buffer

  m_socket.async_read_some(
      asio::buffer(m_buffer.data(), m_buffer.capacity()),
      [this, self](asio::error_code ec, std::size_t length) {
        if (!ec) {
          // Re-use legacy logic temporarily by feeding the bytes to a stream
          // analyzer
          // 1. Process received data
          // (We will invoke the logic synchronously so we can reuse the
          this->onDataReceived(reinterpret_cast<const char *>(m_buffer.data()),
                               length);
        } else if (ec != asio::error::operation_aborted) {
          std::cout << "[Session " << m_sessionId
                    << "] Disconnected: " << ec.message() << std::endl;
          // The connection is dropped. `self` drops out of scope, destroying
          // the session.
        }
      });
}

void ClientSession::onDataReceived(const char *data, size_t length) {
  // Rather than keeping `m_buffer` as the main read destination, we use it as
  // the accumulator
  // 1. Append new incoming bytes to our persistent accumulating buffer (renamed
  // later if needed, using `m_buffer` right now causes a conflict if we async
  // read into it directly) Let's create an `m_receiveBuffer` in the class
  // later, for now we will cheat by statically analyzing it
  static std::vector<uint8_t> accumulator;
  accumulator.insert(accumulator.end(), data, data + length);

  while (true) {
    if (accumulator.size() < sizeof(PacketHeader))
      break;

    uint32_t networkLength;
    std::memcpy(&networkLength, accumulator.data() + 8, sizeof(uint32_t));
    uint32_t bodyLength = ntohl(networkLength);

    size_t totalSize = sizeof(PacketHeader) + bodyLength;
    if (accumulator.size() < totalSize)
      break;

    try {
      std::vector<uint8_t> packetData(accumulator.begin(),
                                      accumulator.begin() + totalSize);
      Packet pkt(packetData);
      processPacket(pkt);
      accumulator.erase(accumulator.begin(), accumulator.begin() + totalSize);
    } catch (const std::exception &e) {
      std::cerr << "[Session " << m_sessionId << "] Data Error: " << e.what()
                << std::endl;
      // Close socket explicitly on error
      asio::error_code closeEc;
      m_socket.close(closeEc);
      return;
    }
  }

  // Chain the next read asynchronously
  doRead();
}

// Legacy signature preserved, now rewritten using the `static accumulator`
// logic in the chunk above

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
        m_onTypingIndicator(shared_from_this(), targetUser, isTyping);
      }
    }
    break;

  case PacketType::UpdateAvatar:
    handleUpdateAvatar(packet);
    break;

  case PacketType::GetAvatar:
    handleGetAvatar(packet);
    break;

  default:
    std::cout << "[Session] Unknown Packet Type: "
              << static_cast<int>(packet.type()) << std::endl;
    break;
  }
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
    m_onMessage(shared_from_this(), targetUser, messageBody);
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

  if (!m_server)
    return;

  m_server->getDb().postTask(
      [server = m_server, sessionId = m_sessionId, username, password]() {
        bool ok = server->getDb().createUser(username, password);

        server->postResponse([server, sessionId, username, ok]() {
          ClientSession *session = server->getSession(sessionId);
          if (!session)
            return;

          if (ok) {
            std::cout << "[Session] Registration SUCCESS for " << username
                      << std::endl;
            session->m_username = username;
            session->m_isLoggedIn = true;

            Packet resp(PacketType::RegisterSuccess);
            resp.writeString("Registration Successful! Welcome, " + username);
            session->sendPacket(resp);

            if (session->m_onLogin) {
              session->m_onLogin(session->shared_from_this());
            }
          } else {
            std::cout << "[Session] Registration FAILED (Taken): " << username
                      << std::endl;
            Packet resp(PacketType::RegisterFailed);
            resp.writeString("Username already taken.");
            session->sendPacket(resp);
          }
        });
      });
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
  if (!m_server)
    return;

  m_server->getDb().postTask([server = m_server, sessionId = m_sessionId,
                              username, password]() {
    bool ok = server->getDb().checkCredentials(username, password);
    std::vector<std::string> friends;
    if (ok) {
      friends = server->getDb().getFriends(username);
    }

    server->postResponse([server, sessionId, username, ok,
                          friends = std::move(friends)]() {
      ClientSession *session = server->getSession(sessionId);
      if (!session)
        return;

      if (ok) {
        std::cout << "[Session] Login SUCCESS for " << username << std::endl;
        session->m_username = username;
        session->m_isLoggedIn = true;

        Packet resp(PacketType::LoginSuccess);
        resp.writeString("Welcome to WizzMania, " + username + "!");
        session->sendPacket(resp);

        if (!friends.empty()) {
          Packet contactList(PacketType::ContactList);
          contactList.writeInt(static_cast<uint32_t>(friends.size()));
          for (const auto &name : friends) {
            contactList.writeString(name);
            int status = session->m_getStatus ? session->m_getStatus(name) : 3;
            contactList.writeInt(static_cast<uint32_t>(status));
          }
          session->sendPacket(contactList);
        }

        if (session->m_onLogin) {
          session->m_onLogin(session->shared_from_this());
        }
      } else {
        std::cout << "[Session] Login FAILED for " << username << std::endl;
        Packet resp(PacketType::LoginFailed);
        resp.writeString("Invalid Username or Password.");
        session->sendPacket(resp);
      }
    });
  });
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

  if (!m_server)
    return;

  m_server->getDb().postTask([server = m_server, sessionId = m_sessionId,
                              username = m_username, targetUser]() {
    bool ok = server->getDb().addFriend(username, targetUser);
    std::vector<std::string> friends;
    if (ok) {
      friends = server->getDb().getFriends(username);
    }

    server->postResponse([server, sessionId, ok,
                          friends = std::move(friends)]() {
      ClientSession *session = server->getSession(sessionId);
      if (!session)
        return;

      if (ok) {
        Packet resp(PacketType::ContactList);
        resp.writeInt(static_cast<uint32_t>(friends.size()));
        for (const auto &name : friends) {
          resp.writeString(name);
          int status = session->m_getStatus ? session->m_getStatus(name) : 3;
          resp.writeInt(static_cast<uint32_t>(status));
        }
        session->sendPacket(resp);
      } else {
        Packet err(PacketType::Error);
        err.writeString("Failed to add contact: User not found.");
        session->sendPacket(err);
      }
    });
  });
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

  if (!m_server)
    return;

  m_server->getDb().postTask([server = m_server, sessionId = m_sessionId,
                              username = m_username, targetUser]() {
    bool ok = server->getDb().removeFriend(username, targetUser);
    std::vector<std::string> friends;
    if (ok) {
      friends = server->getDb().getFriends(username);
    }

    server->postResponse([server, sessionId, ok,
                          friends = std::move(friends)]() {
      ClientSession *session = server->getSession(sessionId);
      if (!session)
        return;

      if (ok) {
        Packet resp(PacketType::ContactList);
        resp.writeInt(static_cast<uint32_t>(friends.size()));
        for (const auto &name : friends) {
          resp.writeString(name);
          int status = session->m_getStatus ? session->m_getStatus(name) : 3;
          resp.writeInt(static_cast<uint32_t>(status));
        }
        session->sendPacket(resp);
      }
    });
  });
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
    m_onStatusChange(shared_from_this(), newStatus);
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
    m_onNudge(shared_from_this(), targetUser);
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
    m_onVoiceMessage(shared_from_this(), targetUser, duration, audioData);
  }
}

void ClientSession::handleUpdateAvatar(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  uint32_t dataLen = 0;
  std::vector<uint8_t> avatarData;

  try {
    dataLen = packet.readInt();
    if (dataLen > 5 * 1024 * 1024) { // 5MB limit
      std::cerr << "[Session] Avatar too large: " << dataLen << std::endl;
      return;
    }
    avatarData = packet.readBytes(dataLen);
  } catch (...) {
    return;
  }

  std::cout << "[Session] Avatar Update: " << m_username << " (" << dataLen
            << " bytes)" << std::endl;

  if (m_onUpdateAvatar) {
    m_onUpdateAvatar(shared_from_this(), avatarData);
  }
}

void ClientSession::handleGetAvatar(Packet &packet) {
  if (!m_isLoggedIn)
    return;

  std::string targetUser;
  try {
    targetUser = packet.readString();
  } catch (...) {
    return;
  }

  // std::cout << "[Session] Request Avatar for: " << targetUser << std::endl;

  if (m_onGetAvatar) {
    m_onGetAvatar(shared_from_this(), targetUser);
  }
}

} // namespace wizz
