#include "TcpServer.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace wizz {

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

                  // Check for Offline Messages
                  auto pending = m_db.fetchPendingMessages(username);
                  if (!pending.empty()) {
                    std::cout << "[Server] Flushing " << pending.size()
                              << " offline messages to " << username
                              << std::endl;
                    for (const auto &msg : pending) {
                      Packet outPacket(PacketType::DirectMessage);
                      outPacket.writeString(msg.sender);
                      outPacket.writeString(msg.body);
                      session->sendPacket(outPacket);

                      // Mark as Delivered
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

                    // Forward Message
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

                  // Store in DB (History + Offline)
                  m_db.storeMessage(sender->getUsername(), target, msg,
                                    delivered);
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

} // namespace wizz
