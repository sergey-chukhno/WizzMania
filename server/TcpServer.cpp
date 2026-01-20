#include "TcpServer.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace wizz {

TcpServer::TcpServer(int port)
    : m_port(port), m_serverSocket(INVALID_SOCKET_VAL), m_isRunning(false) {
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
        // Note: emplace constructs the object in-place (efficient)
        m_sessions.emplace(clientSocket, ClientSession(clientSocket));
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
          // close_socket_raw is called by ClientSession destructor
          // automatically!
          it = m_sessions.erase(it); // Remove from map
        } else {
          // Valid Data Received
          std::string msg(recvBuf, bytesReceived);
          std::cout << "[Server] Received from " << sock << ": " << msg
                    << std::endl;
          ++it;
        }
      } else {
        ++it;
      }
    }
  }
}

} // namespace wizz
