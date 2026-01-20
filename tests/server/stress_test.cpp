#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

// Use our actual Protocol Library
#include "../../common/Packet.h"
#include "../../common/Types.h"

// Platform Compatibility
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
// SocketType is defined in Types.h now, but we need standard headers for
// socket() calls here if not fully abstracting
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
  SocketType sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET_VAL) {
    std::cerr << "[Client] Error creating socket" << std::endl;
    return;
  }

  // 2. Define Server Address
  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

  // 3. Connect
  std::cout << "[Client] Connecting to " << SERVER_IP << ":" << SERVER_PORT
            << "..." << std::endl;
  if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    std::cerr << "[Client] Connection Failed!" << std::endl;
  } else {
    std::cout << "[Client] Connected!" << std::endl;

    // 4. Send Data (Day 3 Update)
    // Construct a Register Packet (Day 4 Test)
    wizz::Packet regPacket(wizz::PacketType::Register);
    regPacket.writeString("TestRegUser"); // Unique User
    regPacket.writeString("Pass123");

    std::vector<uint8_t> buffer = regPacket.serialize();

    std::cout << "[Client] Sending " << buffer.size()
              << " bytes (Registration)..." << std::endl;
    send(sock, reinterpret_cast<const char *>(buffer.data()), buffer.size(), 0);

    // 5. Receive Response
    char recvBuf[1024];
    int bytes = recv(sock, recvBuf, sizeof(recvBuf), 0);
    if (bytes > 0) {
      // In a real client, we would buffer this too.
      // Here we assume the test is on localhost and we get the full packet.
      // Skip Header (12 bytes) to see the string payload roughly
      // Or better: Use Packet class to deserialize
      std::vector<uint8_t> respData(recvBuf, recvBuf + bytes);
      wizz::Packet resp(respData); // Deserializes

      std::cout << "[Client] Response Type: " << static_cast<int>(resp.type())
                << std::endl;
      if (resp.type() == wizz::PacketType::LoginSuccess) {
        std::string msg = resp.readString();
        std::cout << "[Client] SUCCESS Message: " << msg << std::endl;
      } else if (resp.type() == wizz::PacketType::LoginFailed) {
        std::string msg = resp.readString();
        std::cout << "[Client] FAILURE Message: " << msg << std::endl;
      }
    }
  }

  // Cleanup
  close_socket_raw(sock);

#ifdef _WIN32
  WSACleanup();
#endif
}

int main() {
  run_client();
  return 0;
}
