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
    : m_port(port), m_serverSocket(INVALID_SOCKET_VAL), m_isRunning(false),
      m_db("wizzmania.db") // Default DB file
{
// Windows Sockets Initialization (One-time)
#ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    throw std::runtime_error("WSAStartup Failed");
  }
#endif
}

TcpServer::~TcpServer() {
  stop();
#ifdef _WIN32
  WSACleanup();
#endif
}

void TcpServer::start() {
  try {
    // 1. Initialize Database
    if (!m_db.init()) {
      throw std::runtime_error("Failed to initialize Database!");
    }

    setupVoiceStorage(); // Create storage directory

    initSocket();
    bindSocket();
    listenSocket();

    std::cout << "[Server] Listening on port " << m_port << std::endl;
    m_isRunning = true;

    run(); // Enter Main Loop
  } catch (const std::exception &e) {
    std::cerr << "[Server] Fatal Error: " << e.what() << std::endl;
    stop(); // Ensure cleanup
    throw;  // Re-throw to caller
  }
}

void TcpServer::stop() {
  m_isRunning = false;
  if (m_serverSocket != INVALID_SOCKET_VAL) {
    close_socket_raw(m_serverSocket);
    m_serverSocket = INVALID_SOCKET_VAL;
    std::cout << "[Server] Stopped." << std::endl;
  }
}

void TcpServer::initSocket() {
  // 1. Create Socket (IPv4, TCP, Default Protocol)
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  if (m_serverSocket == INVALID_SOCKET_VAL) {
    throw std::runtime_error("Failed to create socket");
  }

  // 2. Set SO_REUSEADDR (Allows immediate restart on same port)
  int opt = 1;
#ifdef _WIN32
  setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
             sizeof(opt));
#else
  setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
}

void TcpServer::bindSocket() {
  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on ALL interfaces (0.0.0.0)
  serverAddr.sin_port =
      htons(static_cast<uint16_t>(m_port)); // Port in Network Byte Order

  if (bind(m_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) <
      0) {
    throw std::runtime_error("Failed to bind socket (Port used?)");
  }
}

void TcpServer::listenSocket() {
  // Backlog of 5 pending connections
  if (listen(m_serverSocket, 5) < 0) {
    throw std::runtime_error("Failed to listen on socket");
  }
}

void TcpServer::run() {
  // Main Event Loop
  while (m_isRunning) {
    fd_set readfds;
    FD_ZERO(&readfds);

    // 1. Monitor Server Socket (for new connections)
    FD_SET(m_serverSocket, &readfds);
    int max_fd = static_cast<int>(m_serverSocket);

    // 2. Monitor All Client Sockets (for incoming data)
    for (const auto &pair : m_sessions) {
      SocketType sock = pair.first;
      FD_SET(sock, &readfds);
      if (static_cast<int>(sock) > max_fd) {
        max_fd = static_cast<int>(sock);
      }
    }

    // Timeout (1 second)
    timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // 3. Wait for Activity
    int activity = select(max_fd + 1, &readfds, nullptr, nullptr, &timeout);

    if (activity < 0) {
      std::cerr << "[Server] Select error" << std::endl;
      break;
    }

    if (activity == 0) {
      continue; // Timeout
    }

    // 4. Check New Connections
    if (FD_ISSET(m_serverSocket, &readfds)) {
      sockaddr_in clientAddr;
      socklen_t clientLen = sizeof(clientAddr);
      SocketType clientSocket =
          accept(m_serverSocket, (struct sockaddr *)&clientAddr, &clientLen);

      if (clientSocket != INVALID_SOCKET_VAL) {
        std::cout << "[Server] New Connection: " << clientSocket << std::endl;

        // Create Session and Move into Map
        // Pass m_db reference AND the Login Callback
        m_sessions.emplace(
            clientSocket,
            ClientSession(
                clientSocket, m_db,
                // 1. OnLogin
                [this](ClientSession *session) {
                  std::string username = session->getUsername();
                  std::cout << "[Server] User Online: " << username
                            << std::endl;
                  m_onlineUsers[username] = session;

                  // Broadcast Online Status to Friends
                  std::vector<std::string> friends = m_db.getFriends(username);
                  for (const auto &friendName : friends) {
                    auto it = m_onlineUsers.find(friendName);
                    if (it != m_onlineUsers.end()) {
                      // 1. Notify Friend that I am Online (Status=1)
                      Packet notify(PacketType::ContactStatusChange);
                      notify.writeInt(1); // Online
                      notify.writeString(username);
                      it->second->sendPacket(notify);
                    }
                  }

                  // Check for Offline Messages
                  auto pending = m_db.fetchPendingMessages(username);
                  if (!pending.empty()) {
                    std::cout << "[Server] Flushing " << pending.size()
                              << " offline messages to " << username
                              << std::endl;
                    for (const auto &msg : pending) {
                      if (msg.body.rfind("VOICE:", 0) == 0) {
                        // It's a voice message!
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
                              outPacket.writeData(buffer.data(), buffer.size());
                              session->sendPacket(outPacket);
                            }
                          }
                        }
                      } else {
                        // Standard Text Message
                        Packet outPacket(PacketType::DirectMessage);
                        outPacket.writeString(msg.sender);
                        outPacket.writeString(msg.body);
                        session->sendPacket(outPacket);
                      }
                      m_db.markAsDelivered(msg.id);
                    }
                  }
                },
                // 2. OnMessage (Routing)
                [this](ClientSession *sender, const std::string &target,
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
                    std::cout << "[Router] Routed msg from "
                              << sender->getUsername() << " to " << target
                              << std::endl;
                  } else {
                    std::cout << "[Router] User " << target
                              << " not found (Offline). Storing." << std::endl;
                  }
                  m_db.storeMessage(sender->getUsername(), target, msg,
                                    delivered);
                },
                // 3. OnNudge Callback
                [this](ClientSession *sender, const std::string &target) {
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
                },
                // 4. OnVoiceMessage Callback
                [this](ClientSession *sender, const std::string &target,
                       uint16_t duration, const std::vector<uint8_t> &data) {
                  long long timestamp = std::time(nullptr);
                  std::stringstream ss;
                  ss << "server/storage/voice_" << sender->getUsername() << "_"
                     << timestamp << ".wav";
                  std::string filepath = ss.str();

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
                    m_db.storeMessage(sender->getUsername(), target, proxyMsg,
                                      false);
                  }
                },
                // 5. OnTypingIndicator Callback
                [this](ClientSession *sender, const std::string &target,
                       bool isTyping) {
                  auto it = m_onlineUsers.find(target);
                  if (it != m_onlineUsers.end()) {
                    ClientSession *targetSession = it->second;
                    Packet p(PacketType::TypingIndicator);
                    p.writeString(sender->getUsername());
                    p.writeInt(isTyping ? 1 : 0);
                    targetSession->sendPacket(p);
                  }
                },
                // 6. GetStatus Callback
                [this](const std::string &username) -> int {
                  if (m_userStatuses.find(username) != m_userStatuses.end()) {
                    return m_userStatuses[username];
                  }
                  if (m_onlineUsers.find(username) != m_onlineUsers.end()) {
                    return 1; // Online default
                  }
                  return 3; // Offline
                },
                // 7. OnStatusChange Callback
                [this](ClientSession *sender, int newStatus) {
                  std::string username = sender->getUsername();
                  m_userStatuses[username] = newStatus;
                  std::vector<std::string> friends = m_db.getFriends(username);
                  for (const auto &friendName : friends) {
                    auto it = m_onlineUsers.find(friendName);
                    if (it != m_onlineUsers.end()) {
                      Packet notify(PacketType::ContactStatusChange);
                      notify.writeInt(static_cast<uint32_t>(newStatus));
                      notify.writeString(username);
                      it->second->sendPacket(notify);
                    }
                  }
                },
                // 8. OnUpdateAvatar Callback
                [this](ClientSession *sender,
                       const std::vector<uint8_t> &data) {
                  std::string username = sender->getUsername();
                  long long timestamp = std::time(nullptr);
                  std::stringstream ss;
                  ss << "server/storage/avatars/avatar_" << username << "_"
                     << timestamp << ".png";
                  std::string filepath = ss.str();

                  if (!fs::exists("server/storage/avatars")) {
                    fs::create_directories("server/storage/avatars");
                  }

                  std::ofstream outfile(filepath, std::ios::binary);
                  if (outfile.is_open()) {
                    outfile.write(reinterpret_cast<const char *>(data.data()),
                                  data.size());
                    outfile.close();
                    std::cout << "[Server] Saved Avatar: " << filepath
                              << std::endl;
                    if (m_db.updateUserAvatar(username, filepath)) {
                      std::cout << "[Server] DB Updated for " << username
                                << std::endl;

                      // BROADCAST TO FRIENDS
                      std::vector<std::string> friends =
                          m_db.getFriends(username);
                      for (const auto &friendName : friends) {
                        auto it = m_onlineUsers.find(friendName);
                        if (it != m_onlineUsers.end()) {
                          // Send AvatarData to friend
                          Packet resp(PacketType::AvatarData);
                          resp.writeString(username);
                          resp.writeInt(static_cast<uint32_t>(data.size()));
                          resp.writeData(data.data(), data.size());
                          it->second->sendPacket(resp);
                          std::cout << "[Server] Broadcasted avatar to "
                                    << friendName << std::endl;
                        }
                      }
                      // Also reflect back to sender to confirm? (Client handles
                      // local update, but good for consistency)
                    } else {
                      std::cerr << "[Server] DB Update Failed for " << username
                                << std::endl;
                    }
                  } else {
                    std::cerr << "[Server] Failed to write file: " << filepath
                              << std::endl;
                  }
                },
                // 9. OnGetAvatar Callback
                [this](ClientSession *sender, const std::string &target) {
                  std::string filepath = m_db.getUserAvatar(target);
                  // Debug Logs
                  std::cout << "[Server] GetAvatar req for " << target
                            << ". Path: " << filepath << std::endl;

                  if (filepath.empty()) {
                    std::cout << "[Server] No avatar path in DB for " << target
                              << std::endl;
                    return;
                  }

                  if (!fs::exists(filepath)) {
                    std::cout << "[Server] File not found: " << filepath
                              << std::endl;
                    return;
                  }

                  std::ifstream infile(filepath,
                                       std::ios::binary | std::ios::ate);
                  if (infile.is_open()) {
                    std::streamsize size = infile.tellg();
                    infile.seekg(0, std::ios::beg);
                    std::vector<uint8_t> buffer(size);
                    if (infile.read(reinterpret_cast<char *>(buffer.data()),
                                    size)) {
                      Packet resp(PacketType::AvatarData);
                      resp.writeString(target);
                      resp.writeInt(static_cast<uint32_t>(buffer.size()));
                      resp.writeData(buffer.data(), buffer.size());
                      sender->sendPacket(resp);
                      std::cout << "[Server] Sent avatar (" << size
                                << " bytes) to " << sender->getUsername()
                                << std::endl;
                    }
                  } else {
                    std::cerr
                        << "[Server] Failed to open file for read: " << filepath
                        << std::endl;
                  }
                }));
      }
    }

    // 5. Check Data from Existing Clients
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
      SocketType sock = it->first;

      if (FD_ISSET(sock, &readfds)) {
        char recvBuf[1024];
        int bytesReceived = recv(sock, recvBuf, sizeof(recvBuf), 0);
        if (bytesReceived <= 0) {
          // Disconnected or Error
          std::cout << "[Server] Client Disconnected: " << sock << std::endl;

          // Day 4: Remove from Registry if logged in
          ClientSession &session = it->second;
          // Note: We need to know if they were logged in.
          // Ideally ClientSession had isLoggedIn() or we check username empty?
          std::string username = session.getUsername();
          if (!username.empty()) {
            m_onlineUsers.erase(username);
            std::cout << "[Server] User Offline: " << username << std::endl;

            // Broadcast Offline Status (3) to Friends
            std::vector<std::string> friends = m_db.getFriends(username);
            for (const auto &friendName : friends) {
              auto it = m_onlineUsers.find(friendName);
              if (it != m_onlineUsers.end()) {
                Packet notify(PacketType::ContactStatusChange);
                notify.writeInt(3); // Offline
                notify.writeString(username);
                it->second->sendPacket(notify);
              }
            }
          }

          it = m_sessions.erase(it); // Remove from map
        } else {
          // Valid Data Received -> Pass to Session
          ClientSession &session = it->second;
          bool keepAlive = session.onDataReceived(recvBuf, bytesReceived);

          if (!keepAlive) {
            std::cout << "[Server] Kicking Client (Protocol Error): " << sock
                      << std::endl;
            it = m_sessions.erase(it);
          } else {
            ++it;
          }
        }
      } else {
        ++it;
      }
    }
  }
}

void TcpServer::setupVoiceStorage() {
  if (!fs::exists("server/storage")) {
    fs::create_directories("server/storage");
    std::cout << "[Server] Created storage directory: server/storage"
              << std::endl;
  }
}

} // namespace wizz
