import re

with open("server/TcpServer.cpp", "r") as f:
    content = f.read()

# Append the implementations
implementations = """
void TcpServer::handleLogin(ClientSession *session) {
  std::string username = session->getUsername();
  SocketType socket = session->getSocket();

  m_db.postTask([this, username, socket]() {
    auto pending = m_db.fetchPendingMessages(username);
    for (const auto &msg : pending) {
      m_db.markAsDelivered(msg.id);
    }
    auto friends = m_db.getFriends(username);

    postResponse([this, username, socket,
                  pending = std::move(pending),
                  friends = std::move(friends)]() {
      ClientSession *session = getSession(socket);
      if (!session)
        return;

      std::cout << "[Server] User Online: " << username
                << std::endl;
      m_onlineUsers[username] = session;

      // Broadcast Online Status to Friends
      for (const auto &friendName : friends) {
        auto it = m_onlineUsers.find(friendName);
        if (it != m_onlineUsers.end()) {
          Packet notify(PacketType::ContactStatusChange);
          notify.writeInt(1); // Online
          notify.writeString(username);
          it->second->sendPacket(notify);
        }
      }

      // Check for Offline Messages
      if (!pending.empty()) {
        std::cout << "[Server] Flushing " << pending.size()
                  << " offline messages to " << username
                  << std::endl;
        for (const auto &msg : pending) {
          if (msg.body.rfind("VOICE:", 0) == 0) {
            std::vector<std::string> parts;
            std::stringstream ss(msg.body);
            std::string item;
            while (std::getline(ss, item, ':')) {
              parts.push_back(item);
            }
            if (parts.size() >= 3) {
              uint16_t duration =
                  static_cast<uint16_t>(std::stoi(parts[1]));
              std::string filename = parts[2];
              std::ifstream infile(filename, std::ios::binary |
                                                 std::ios::ate);
              if (infile.is_open()) {
                std::streamsize size = infile.tellg();
                infile.seekg(0, std::ios::beg);
                std::vector<uint8_t> buffer(size);
                if (infile.read(
                        reinterpret_cast<char *>(buffer.data()),
                        size)) {
                  Packet outPacket(PacketType::VoiceMessage);
                  outPacket.writeString(msg.sender);
                  outPacket.writeInt(
                      static_cast<uint32_t>(duration));
                  outPacket.writeInt(
                      static_cast<uint32_t>(buffer.size()));
                  outPacket.writeData(buffer.data(),
                                      buffer.size());
                  session->sendPacket(outPacket);
                }
              }
            }
          } else {
            Packet outPacket(PacketType::DirectMessage);
            outPacket.writeString(msg.sender);
            outPacket.writeString(msg.body);
            session->sendPacket(outPacket);
          }
        }
      }
    });
  });
}

void TcpServer::handleMessage(ClientSession *sender, const std::string &target, const std::string &msg) {
  bool delivered = false;
  auto it = m_onlineUsers.find(target);
  if (it != m_onlineUsers.end()) {
    ClientSession *targetSession = it->second;
    Packet outPacket(PacketType::DirectMessage);
    outPacket.writeString(sender->getUsername());
    outPacket.writeString(msg);
    targetSession->sendPacket(outPacket);
    delivered = true;
    std::cout << "[Router] Routed msg from "
              << sender->getUsername() << " to " << target
              << std::endl;
  } else {
    std::cout << "[Router] User " << target
              << " not found (Offline). Storing." << std::endl;
  }

  // Fire and forget DB insertion
  m_db.postTask([this, senderName = sender->getUsername(),
                 target, msg, delivered]() {
    m_db.storeMessage(senderName, target, msg, delivered);
  });
}

void TcpServer::handleNudge(ClientSession *sender, const std::string &target) {
  int status = 3; // Default Offline
  if (m_userStatuses.find(target) != m_userStatuses.end()) {
    status = m_userStatuses[target];
  }
  bool isOnline =
      m_onlineUsers.find(target) != m_onlineUsers.end();

  if (!isOnline) {
    Packet err(PacketType::Error);
    err.writeString("User " + target + " is offline.");
    sender->sendPacket(err);
    return;
  }
  if (status == 2) { // Busy
    Packet err(PacketType::Error);
    err.writeString("User " + target +
                    " is busy and cannot be nudged.");
    sender->sendPacket(err);
    return;
  }
  ClientSession *targetSession = m_onlineUsers[target];
  Packet p(PacketType::Nudge);
  p.writeString(sender->getUsername());
  targetSession->sendPacket(p);
  std::cout << "[Server] Wizz sent from "
            << sender->getUsername() << " to " << target
            << std::endl;
}

void TcpServer::handleVoiceMessage(ClientSession *sender, const std::string &target, uint16_t duration, const std::vector<uint8_t> &data) {
  long long timestamp = std::time(nullptr);
  fs::path storageDir = fs::path("server") / "storage";
  std::string filename = "voice_" + sender->getUsername() +
                         "_" + std::to_string(timestamp) +
                         ".wav";
  std::string filepath = (storageDir / filename).string();

  std::ofstream outfile(filepath, std::ios::binary);
  if (outfile.is_open()) {
    outfile.write(reinterpret_cast<const char *>(data.data()),
                  data.size());
    outfile.close();
  }

  auto it = m_onlineUsers.find(target);
  if (it != m_onlineUsers.end()) {
    ClientSession *targetSession = it->second;
    Packet p(PacketType::VoiceMessage);
    p.writeString(sender->getUsername());
    p.writeInt(static_cast<uint32_t>(duration));
    p.writeInt(static_cast<uint32_t>(data.size()));
    p.writeData(data.data(), data.size());
    targetSession->sendPacket(p);
  } else {
    std::string proxyMsg =
        "VOICE:" + std::to_string(duration) + ":" + filepath;
    m_db.postTask([this, senderName = sender->getUsername(),
                   target, proxyMsg]() {
      m_db.storeMessage(senderName, target, proxyMsg, false);
    });
  }
}

void TcpServer::handleTypingIndicator(ClientSession *sender, const std::string &target, bool isTyping) {
  auto it = m_onlineUsers.find(target);
  if (it != m_onlineUsers.end()) {
    ClientSession *targetSession = it->second;
    Packet p(PacketType::TypingIndicator);
    p.writeString(sender->getUsername());
    p.writeInt(isTyping ? 1 : 0);
    targetSession->sendPacket(p);
  }
}

int TcpServer::handleGetStatus(const std::string &username) {
  if (m_userStatuses.find(username) != m_userStatuses.end()) {
    return m_userStatuses[username];
  }
  if (m_onlineUsers.find(username) != m_onlineUsers.end()) {
    return 1; // Online default
  }
  return 3; // Offline
}

void TcpServer::handleStatusChange(ClientSession *sender, int newStatus) {
  std::string username = sender->getUsername();
  m_userStatuses[username] = newStatus;

  m_db.postTask([this, username, newStatus]() {
    auto friends = m_db.getFriends(username);
    postResponse([this, username, newStatus,
                  friends = std::move(friends)]() {
      for (const auto &friendName : friends) {
        auto it = m_onlineUsers.find(friendName);
        if (it != m_onlineUsers.end()) {
          Packet notify(PacketType::ContactStatusChange);
          notify.writeInt(static_cast<uint32_t>(newStatus));
          notify.writeString(username);
          it->second->sendPacket(notify);
        }
      }
    });
  });
}

void TcpServer::handleUpdateAvatar(ClientSession *sender, const std::vector<uint8_t> &data) {
  std::string username = sender->getUsername();
  long long timestamp = std::time(nullptr);
  fs::path storageDir =
      fs::path("server") / "storage" / "avatars";

  if (!fs::exists(storageDir)) {
    fs::create_directories(storageDir);
  }

  std::string filename = "avatar_" + username + "_" +
                         std::to_string(timestamp) + ".png";
  std::string filepath = (storageDir / filename).string();

  std::ofstream outfile(filepath, std::ios::binary);
  if (outfile.is_open()) {
    outfile.write(reinterpret_cast<const char *>(data.data()),
                  data.size());
    outfile.close();
    std::cout << "[Server] Saved Avatar: " << filepath
              << std::endl;
    m_db.postTask([this, username, filepath, data]() {
      if (m_db.updateUserAvatar(username, filepath)) {
        std::cout << "[Server] DB Updated for " << username
                  << std::endl;
        auto friends = m_db.getFriends(username);

        postResponse([this, username,
                      friends = std::move(friends), data]() {
          for (const auto &friendName : friends) {
            auto it = m_onlineUsers.find(friendName);
            if (it != m_onlineUsers.end()) {
              Packet resp(PacketType::AvatarData);
              resp.writeString(username);
              resp.writeInt(static_cast<uint32_t>(data.size()));
              resp.writeData(data.data(), data.size());
              it->second->sendPacket(resp);
              std::cout << "[Server] Broadcasted avatar to "
                        << friendName << std::endl;
            }
          }
        });
      } else {
        std::cerr << "[Server] DB Update Failed for "
                  << username << std::endl;
      }
    });
  } else {
    std::cerr << "[Server] Failed to write file: " << filepath
              << std::endl;
  }
}

void TcpServer::handleGetAvatar(ClientSession *sender, const std::string &target) {
  SocketType socket = sender->getSocket();

  m_db.postTask([this, socket, target]() {
    std::string filepath = m_db.getUserAvatar(target);
    std::vector<uint8_t> buffer;
    if (!filepath.empty() && fs::exists(filepath)) {
      std::ifstream infile(filepath,
                           std::ios::binary | std::ios::ate);
      if (infile.is_open()) {
        std::streamsize size = infile.tellg();
        infile.seekg(0, std::ios::beg);
        buffer.resize(size);
        infile.read(reinterpret_cast<char *>(buffer.data()),
                    size);
      }
    }

    postResponse([this, socket, target, filepath,
                  buffer = std::move(buffer)]() {
      ClientSession *session = getSession(socket);
      if (!session)
        return;

      std::cout << "[Server] GetAvatar req for " << target
                << ". Path: " << filepath << std::endl;
      if (filepath.empty()) {
        std::cout << "[Server] No avatar path in DB for "
                  << target << std::endl;
        return;
      }
      if (buffer.empty()) {
        std::cerr << "[Server] Failed to open file for read or "
                     "file empty: "
                  << filepath << std::endl;
        return;
      }

      Packet resp(PacketType::AvatarData);
      resp.writeString(target);
      resp.writeInt(static_cast<uint32_t>(buffer.size()));
      resp.writeData(buffer.data(), buffer.size());
      session->sendPacket(resp);
      std::cout << "[Server] Sent avatar (" << buffer.size()
                << " bytes) to " << session->getUsername()
                << std::endl;
    });
  });
}
"""

content += "\n" + implementations

# Now we slice out the big block in run().
# The start is:
#                // 1. OnLogin
#                [this](ClientSession *session) {
# The end is:
#                }));
#      }
#    }

start_idx = content.find('                // 1. OnLogin')
end_idx = content.find('                }));', start_idx) + len('                }));')

assert start_idx != -1 and end_idx > start_idx

new_lambdas = """                // 1. OnLogin
                [this](ClientSession *session) { handleLogin(session); },
                // 2. OnMessage (Routing)
                [this](ClientSession *sender, const std::string &target, const std::string &msg) { handleMessage(sender, target, msg); },
                // 3. OnNudge Callback
                [this](ClientSession *sender, const std::string &target) { handleNudge(sender, target); },
                // 4. OnVoiceMessage Callback
                [this](ClientSession *sender, const std::string &target, uint16_t duration, const std::vector<uint8_t> &data) { handleVoiceMessage(sender, target, duration, data); },
                // 5. OnTypingIndicator Callback
                [this](ClientSession *sender, const std::string &target, bool isTyping) { handleTypingIndicator(sender, target, isTyping); },
                // 6. GetStatus Callback
                [this](const std::string &username) -> int { return handleGetStatus(username); },
                // 7. OnStatusChange Callback
                [this](ClientSession *sender, int newStatus) { handleStatusChange(sender, newStatus); },
                // 8. OnUpdateAvatar Callback
                [this](ClientSession *sender, const std::vector<uint8_t> &data) { handleUpdateAvatar(sender, data); },
                // 9. OnGetAvatar Callback
                [this](ClientSession *sender, const std::string &target) { handleGetAvatar(sender, target); }));"""

new_content = content[:start_idx] + new_lambdas + content[end_idx:]

with open("server/TcpServer.cpp", "w") as f:
    f.write(new_content)

print(f"Successfully processed TcpServer.cpp. Old size: {len(content)}, New size: {len(new_content)}")
