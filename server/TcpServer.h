#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Platform Specifics
#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SocketType;
#define INVALID_SOCKET_VAL INVALID_SOCKET
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int SocketType;
#define INVALID_SOCKET_VAL -1
#endif

namespace wizz {

class TcpServer {
public:
  explicit TcpServer(int port);
  ~TcpServer();

  // Prevent copying (Server owns the socket resource uniquely)
  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  // Initialize, Bind, Listen, and Run the loop
  void start();

  // The Main Loop (Blocking)
  // Runs until stop() is called or error occurs
  void run();

  void stop();

private:
  int m_port;
  SocketType m_serverSocket;
  bool m_isRunning;

  // Helpers
  void initSocket();   // Step 1: Create
  void bindSocket();   // Step 2: Bind
  void listenSocket(); // Step 3: Listen
  void cleanup();      // Close and cleanup
};

} // namespace wizz
