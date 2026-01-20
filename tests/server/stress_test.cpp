#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

// Portability (Copy-Paste from TcpServer for now, to keep it standalone)
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Constants
const std::string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;

void run_client() {
#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

  // 1. Create Socket
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET) {
    std::cerr << "[Client] Error creating socket" << std::endl;
    return;
  }

  // 2. Define Server Address
  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

  // 3. Connect (The Client version of "Bind")
  std::cout << "[Client] Connecting to " << SERVER_IP << ":" << SERVER_PORT
            << "..." << std::endl;
  if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
      SOCKET_ERROR) {
    std::cerr << "[Client] Connection Failed! Is the server running?"
              << std::endl;
  } else {
    std::cout << "[Client] Connected! Waiting 1s..." << std::endl;
    // Keep connection alive for a moment to see it on Server side
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "[Client] Disconnecting." << std::endl;
  }

// 4. Cleanup
#ifdef _WIN32
  closesocket(sock);
  WSACleanup();
#else
  close(sock);
#endif
}

int main() {
  // Run one client
  run_client();
  return 0;
}
