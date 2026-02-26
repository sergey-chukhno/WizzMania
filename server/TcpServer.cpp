#include "TcpServer.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace wizz {

namespace fs = std::filesystem;

TcpServer::TcpServer(int port)
    : m_acceptor(m_ioContext,
                 asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      m_sslContext(asio::ssl::context::tlsv12), m_port(port),
      m_isRunning(false), m_db("wizzmania.db") {
  m_sslContext.set_options(asio::ssl::context::default_workarounds |
                           asio::ssl::context::no_sslv2 |
                           asio::ssl::context::single_dh_use);
  m_sslContext.use_certificate_chain_file("server/certs/server.crt");
  m_sslContext.use_private_key_file("server/certs/server.key",
                                    asio::ssl::context::pem);
}

TcpServer::~TcpServer() { stop(); }

void TcpServer::start() {
  try {
    // 1. Initialize Database
    if (!m_db.init()) {
      throw std::runtime_error("Failed to initialize Database!");
    }

    setupVoiceStorage(); // Create storage directory

    std::cout << "[Server] Listening on port " << m_port << std::endl;
    m_isRunning = true;

    // Start accepting connections
    doAccept();

    // 2. Run the Main Event Loop
    run();
  } catch (const std::exception &e) {
    std::cerr << "[Server] Fatal Error: " << e.what() << std::endl;
    stop(); // Ensure cleanup
    throw;  // Re-throw to caller
  }
}

void TcpServer::stop() {
  m_isRunning = false;
  m_ioContext.stop(); // Stop Boost.Asio event loop
  std::cout << "[Server] Stopped." << std::endl;
}

ClientSession *TcpServer::getSession(int sessionId) {
  auto it = m_sessions.find(sessionId);
  if (it != m_sessions.end()) {
    return it->second.get();
  }
  return nullptr;
}

void TcpServer::doAccept() {
  m_acceptor.async_accept([this](asio::error_code ec,
                                 asio::ip::tcp::socket socket) {
    if (!ec) {
      int sessionId = m_nextSessionId++;
      std::cout << "[Server] New Connection (Session ID: " << sessionId << ")"
                << std::endl;

      // Create the Session wrapped in a shared_ptr
      auto session = std::make_shared<ClientSession>(
          sessionId, std::move(socket), m_sslContext, this,
          // 1. OnLogin
          [this](std::shared_ptr<ClientSession> s) { handleLogin(s.get()); },
          // 2. OnMessage (Routing)
          [this](std::shared_ptr<ClientSession> sender,
                 const std::string &target, const std::string &msg) {
            handleMessage(sender.get(), target, msg);
          },
          // 3. OnNudge Callback
          [this](std::shared_ptr<ClientSession> sender,
                 const std::string &target) {
            handleNudge(sender.get(), target);
          },
          // 4. OnVoiceMessage Callback
          [this](std::shared_ptr<ClientSession> sender,
                 const std::string &target, uint16_t duration,
                 const std::vector<uint8_t> &data) {
            handleVoiceMessage(sender.get(), target, duration, data);
          },
          // 5. OnTypingIndicator Callback
          [this](std::shared_ptr<ClientSession> sender,
                 const std::string &target, bool isTyping) {
            handleTypingIndicator(sender.get(), target, isTyping);
          },
          // 6. GetStatus Callback
          [this](const std::string &username) -> int {
            return handleGetStatus(username);
          },
          // 7. OnStatusChange Callback
          [this](std::shared_ptr<ClientSession> sender, int newStatus) {
            handleStatusChange(sender.get(), newStatus);
          },
          // 8. OnUpdateAvatar Callback
          [this](std::shared_ptr<ClientSession> sender,
                 const std::vector<uint8_t> &data) {
            handleUpdateAvatar(sender.get(), data);
          },
          // 9. OnGetAvatar Callback
          [this](std::shared_ptr<ClientSession> sender,
                 const std::string &target) {
            handleGetAvatar(sender.get(), target);
          },
          // 10. OnGameStatus Callback
          [this](std::shared_ptr<ClientSession> sender,
                 const std::string &gameName, uint32_t score) {
            handleGameStatus(sender.get(), gameName, score);
          },
          // 11. OnDisconnect Callback
          [this](int sessionId) { handleDisconnect(sessionId); });

      m_sessions[sessionId] = session;
      session->start(); // Begin reading asynchronously

      // Queue the next accept
      doAccept();
    } else {
      std::cerr << "[Server] Accept Error: " << ec.message() << std::endl;
    }
  });
}

void TcpServer::run() {
  // Main Event Loop is now handled by Boost.Asio
  while (m_isRunning) {
    try {
      m_ioContext.run(); // Block and process all events until stopped
      break;             // `run()` exits normally when stopped or out of work
    } catch (std::exception &e) {
      std::cerr << "[Server] io_context exception: " << e.what() << std::endl;
    }
  }
}

void TcpServer::setupVoiceStorage() {
  fs::path storageDir = fs::path("server") / "storage";
  if (!fs::exists(storageDir)) {
    fs::create_directories(storageDir);
    std::cout << "[Server] Created storage directory: " << storageDir.string()
              << std::endl;
  }
}

} // namespace wizz

void wizz::TcpServer::handleLogin(ClientSession *session) {
  std::string username = session->getUsername();
  int sessionId = session->getId();

  m_db.postTask([this, username, sessionId]() {
    auto pending = m_db.fetchPendingMessages(username);
    for (const auto &msg : pending) {
      m_db.markAsDelivered(msg.id);
    }
    auto followers = m_db.getFollowers(username);

    postResponse([this, username, sessionId, pending = std::move(pending),
                  followers = std::move(followers)]() {
      ClientSession *session = getSession(sessionId);
      if (!session)
        return;

      std::cout << "[Server] User Online: " << username << std::endl;
      m_onlineUsers[username] = session;

      // Broadcast Online Status to Followers
      for (const auto &followerName : followers) {
        auto it = m_onlineUsers.find(followerName);
        if (it != m_onlineUsers.end()) {
          Packet notify(PacketType::ContactStatusChange);
          notify.writeInt(0); // Online
          notify.writeString(username);
          it->second->sendPacket(notify);
        }
      }

      // Check for Offline Messages
      if (!pending.empty()) {
        std::cout << "[Server] Flushing " << pending.size()
                  << " offline messages to " << username << std::endl;
        for (const auto &msg : pending) {
          if (msg.body.rfind("VOICE:", 0) == 0) {
            std::vector<std::string> parts;
            std::stringstream ss(msg.body);
            std::string item;
            while (std::getline(ss, item, ':')) {
              parts.push_back(item);
            }
            if (parts.size() >= 3) {
              uint16_t duration = static_cast<uint16_t>(std::stoi(parts[1]));
              std::string filename = parts[2];
              std::ifstream infile(filename, std::ios::binary | std::ios::ate);
              if (infile.is_open()) {
                std::streamsize size = infile.tellg();
                infile.seekg(0, std::ios::beg);
                std::vector<uint8_t> buffer(size);
                if (infile.read(reinterpret_cast<char *>(buffer.data()),
                                size)) {
                  Packet outPacket(PacketType::VoiceMessage);
                  outPacket.writeString(msg.sender);
                  outPacket.writeInt(static_cast<uint32_t>(duration));
                  outPacket.writeInt(static_cast<uint32_t>(buffer.size()));
                  outPacket.writeData(buffer.data(), buffer.size());
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

void wizz::TcpServer::handleMessage(ClientSession *sender,
                                    const std::string &target,
                                    const std::string &msg) {
  bool delivered = false;
  auto it = m_onlineUsers.find(target);
  if (it != m_onlineUsers.end()) {
    ClientSession *targetSession = it->second;
    Packet outPacket(PacketType::DirectMessage);
    outPacket.writeString(sender->getUsername());
    outPacket.writeString(msg);
    targetSession->sendPacket(outPacket);
    delivered = true;
    std::cout << "[Router] Routed msg from " << sender->getUsername() << " to "
              << target << std::endl;
  } else {
    std::cout << "[Router] User " << target << " not found (Offline). Storing."
              << std::endl;
  }

  // Fire and forget DB insertion
  m_db.postTask(
      [this, senderName = sender->getUsername(), target, msg, delivered]() {
        m_db.storeMessage(senderName, target, msg, delivered);
      });
}

void wizz::TcpServer::handleNudge(ClientSession *sender,
                                  const std::string &target) {
  int status = 3; // Default Offline
  if (m_userStatuses.find(target) != m_userStatuses.end()) {
    status = m_userStatuses[target];
  }
  bool isOnline = m_onlineUsers.find(target) != m_onlineUsers.end();

  if (!isOnline) {
    Packet err(PacketType::Error);
    err.writeString("User " + target + " is offline.");
    sender->sendPacket(err);
    return;
  }
  if (status == 2) { // Busy
    Packet err(PacketType::Error);
    err.writeString("User " + target + " is busy and cannot be nudged.");
    sender->sendPacket(err);
    return;
  }
  ClientSession *targetSession = m_onlineUsers[target];
  Packet p(PacketType::Nudge);
  p.writeString(sender->getUsername());
  targetSession->sendPacket(p);
  std::cout << "[Server] Wizz sent from " << sender->getUsername() << " to "
            << target << std::endl;
}

void wizz::TcpServer::handleVoiceMessage(ClientSession *sender,
                                         const std::string &target,
                                         uint16_t duration,
                                         const std::vector<uint8_t> &data) {
  long long timestamp = std::time(nullptr);
  fs::path storageDir = fs::path("server") / "storage";
  std::string filename = "voice_" + sender->getUsername() + "_" +
                         std::to_string(timestamp) + ".wav";
  std::string filepath = (storageDir / filename).string();

  std::ofstream outfile(filepath, std::ios::binary);
  if (outfile.is_open()) {
    outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
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
    std::string proxyMsg = "VOICE:" + std::to_string(duration) + ":" + filepath;
    m_db.postTask(
        [this, senderName = sender->getUsername(), target, proxyMsg]() {
          m_db.storeMessage(senderName, target, proxyMsg, false);
        });
  }
}

void wizz::TcpServer::handleTypingIndicator(ClientSession *sender,
                                            const std::string &target,
                                            bool isTyping) {
  auto it = m_onlineUsers.find(target);
  if (it != m_onlineUsers.end()) {
    ClientSession *targetSession = it->second;
    Packet p(PacketType::TypingIndicator);
    p.writeString(sender->getUsername());
    p.writeInt(isTyping ? 1 : 0);
    targetSession->sendPacket(p);
  }
}

int wizz::TcpServer::handleGetStatus(const std::string &username) {
  if (m_userStatuses.find(username) != m_userStatuses.end()) {
    return m_userStatuses[username];
  }
  if (m_onlineUsers.find(username) != m_onlineUsers.end()) {
    return 0; // Online default (UserStatus::Online = 0)
  }
  return 3; // Offline (UserStatus::Offline = 3)
}

void wizz::TcpServer::handleStatusChange(ClientSession *sender, int newStatus) {
  std::string username = sender->getUsername();
  m_userStatuses[username] = newStatus;

  m_db.postTask([this, username, newStatus]() {
    auto followers = m_db.getFollowers(username);
    postResponse(
        [this, username, newStatus, followers = std::move(followers)]() {
          for (const auto &followerName : followers) {
            auto it = m_onlineUsers.find(followerName);
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

void wizz::TcpServer::handleUpdateAvatar(ClientSession *sender,
                                         const std::vector<uint8_t> &data) {
  std::string username = sender->getUsername();
  long long timestamp = std::time(nullptr);
  fs::path storageDir = fs::path("server") / "storage" / "avatars";

  if (!fs::exists(storageDir)) {
    fs::create_directories(storageDir);
  }

  std::string filename =
      "avatar_" + username + "_" + std::to_string(timestamp) + ".png";
  std::string filepath = (storageDir / filename).string();

  std::ofstream outfile(filepath, std::ios::binary);
  if (outfile.is_open()) {
    outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
    outfile.close();
    std::cout << "[Server] Saved Avatar: " << filepath << std::endl;
    m_db.postTask([this, username, filepath, data]() {
      if (m_db.updateUserAvatar(username, filepath)) {
        std::cout << "[Server] DB Updated for " << username << std::endl;
        auto friends = m_db.getFriends(username);

        postResponse([this, username, friends = std::move(friends), data]() {
          for (const auto &friendName : friends) {
            auto it = m_onlineUsers.find(friendName);
            if (it != m_onlineUsers.end()) {
              Packet resp(PacketType::AvatarData);
              resp.writeString(username);
              resp.writeInt(static_cast<uint32_t>(data.size()));
              resp.writeData(data.data(), data.size());
              it->second->sendPacket(resp);
              std::cout << "[Server] Broadcasted avatar to " << friendName
                        << std::endl;
            }
          }
        });
      } else {
        std::cerr << "[Server] DB Update Failed for " << username << std::endl;
      }
    });
  } else {
    std::cerr << "[Server] Failed to write file: " << filepath << std::endl;
  }
}

void wizz::TcpServer::handleGetAvatar(ClientSession *sender,
                                      const std::string &target) {
  int sessionId = sender->getId();

  m_db.postTask([this, sessionId, target]() {
    std::string filepath = m_db.getUserAvatar(target);
    std::vector<uint8_t> buffer;
    if (!filepath.empty() && fs::exists(filepath)) {
      std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
      if (infile.is_open()) {
        std::streamsize size = infile.tellg();
        infile.seekg(0, std::ios::beg);
        buffer.resize(size);
        infile.read(reinterpret_cast<char *>(buffer.data()), size);
      }
    }

    postResponse(
        [this, sessionId, target, filepath, buffer = std::move(buffer)]() {
          ClientSession *session = getSession(sessionId);
          if (!session)
            return;

          std::cout << "[Server] GetAvatar req for " << target
                    << ". Path: " << filepath << std::endl;
          if (filepath.empty()) {
            std::cout << "[Server] No avatar path in DB for " << target
                      << std::endl;
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
                    << " bytes) to " << session->getUsername() << std::endl;
        });
  });
}

void wizz::TcpServer::handleGameStatus(ClientSession *sender,
                                       const std::string &gameName,
                                       uint32_t score) {
  if (!sender)
    return;
  std::string username = sender->getUsername();

  Packet pkt(PacketType::GameStatus);
  pkt.writeString(username);
  pkt.writeString(gameName);
  pkt.writeInt(score);

  // Broadcast
  std::vector<std::string> friends = m_db.getFriends(username);
  for (const auto &friendName : friends) {
    auto it = m_onlineUsers.find(friendName);
    if (it != m_onlineUsers.end()) {
      it->second->sendPacket(pkt);
    }
  }
}

void wizz::TcpServer::handleDisconnect(int sessionId) {
  auto it = m_sessions.find(sessionId);
  if (it == m_sessions.end())
    return;

  std::string username = it->second->getUsername();
  if (!username.empty()) {
    m_db.postTask([this, username, sessionId]() {
      auto followers = m_db.getFollowers(username);

      postResponse(
          [this, username, sessionId, followers = std::move(followers)]() {
            // Erase from sessions
            auto sessionIt = m_sessions.find(sessionId);
            if (sessionIt != m_sessions.end()) {
              m_sessions.erase(sessionIt);
            }

            // Erase from online users and broadcast if they were online
            auto onlineIt = m_onlineUsers.find(username);
            if (onlineIt != m_onlineUsers.end()) {
              m_onlineUsers.erase(onlineIt);
              m_userStatuses[username] = 3; // Offline
              std::cout << "[Server] User Offline (Disconnected): " << username
                        << std::endl;

              for (const auto &followerName : followers) {
                auto fIt = m_onlineUsers.find(followerName);
                if (fIt != m_onlineUsers.end()) {
                  Packet notify(PacketType::ContactStatusChange);
                  notify.writeInt(3); // Offline
                  notify.writeString(username);
                  fIt->second->sendPacket(notify);
                }
              }
            }
          });
    });
  } else {
    m_sessions.erase(it);
  }
}
