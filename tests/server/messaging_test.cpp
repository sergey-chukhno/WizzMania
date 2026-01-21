#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#include "../../common/Packet.h"

// Helper: Cross-platform close
void close_socket(SOCKET s) {
#ifdef _WIN32
  closesocket(s);
#else
  close(s);
#endif
}

// Helper: Connect to Server
SOCKET connect_to_server() {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET)
    return INVALID_SOCKET;

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(8080);
  if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
    return INVALID_SOCKET;

  if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    close_socket(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

// Helper: Send Packet
void send_packet(SOCKET sock, const wizz::Packet &p) {
  std::vector<uint8_t> buffer = p.serialize();
  send(sock, reinterpret_cast<const char *>(buffer.data()), buffer.size(), 0);
}

// Helper: Receive specific packet type safely
bool receive_packet_type(SOCKET sock, wizz::PacketType expectedType,
                         std::string *outString1 = nullptr,
                         std::string *outString2 = nullptr) {
  char buffer[1024];
  // Simple blocking recv for test
  int bytes = recv(sock, buffer, sizeof(buffer), 0);
  if (bytes <= 0)
    return false;

  std::vector<uint8_t> data(buffer, buffer + bytes);
  try {
    // Warning: This simplistic test logic assumes 1 packet = 1 recv.
    // In prod, use the buffering logic from ClientSession.
    wizz::Packet p(data);
    if (p.type() == expectedType) {
      if (outString1)
        *outString1 = p.readString();
      if (outString2)
        *outString2 = p.readString();
      return true;
    }
    std::cout << "Received unexpected type: " << (int)p.type() << std::endl;
  } catch (...) {
    return false;
  }
  return false;
}

std::atomic<bool> g_testPassed{false};

void receiver_thread() {
  SOCKET sock = connect_to_server();
  if (sock == INVALID_SOCKET) {
    std::cerr << "[Receiver] Failed to connect." << std::endl;
    return;
  }

  // Login as Sergey
  wizz::Packet login(wizz::PacketType::Login);
  login.writeString("Sergey");
  login.writeString("Password123!");
  send_packet(sock, login);

  // Expect LoginSuccess
  std::string msg;
  if (receive_packet_type(sock, wizz::PacketType::LoginSuccess, &msg)) {
    std::cout << "[Receiver] Logged in as Sergey." << std::endl;
  } else {
    std::cerr << "[Receiver] Login Failed." << std::endl;
    close_socket(sock);
    return;
  }

  // Wait for DirectMessage
  std::cout << "[Receiver] Waiting for message..." << std::endl;
  std::string sender, body;
  // We might need a small loop because recv might return partials,
  // but for this localhost test, small packets usually arrive in one chunk.
  if (receive_packet_type(sock, wizz::PacketType::DirectMessage, &sender,
                          &body)) {
    std::cout << "[Receiver] Got Message from " << sender << ": " << body
              << std::endl;
    if (body == "Hello Sergey!") {
      g_testPassed = true;
    }
  }

  close_socket(sock);
}

void sender_thread() {
  // Wait a bit for Receiver to be online
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  SOCKET sock = connect_to_server();
  if (sock == INVALID_SOCKET)
    return;

  // Register a fresh user to ensure unique
  std::string username = "Sender" + std::to_string(std::rand() % 1000);

  wizz::Packet reg(wizz::PacketType::Register);
  reg.writeString(username);
  reg.writeString("pass");
  send_packet(sock, reg);

  std::string msg;
  // Expect LoginSuccess (Auto-login)
  if (receive_packet_type(sock, wizz::PacketType::LoginSuccess, &msg)) {
    std::cout << "[Sender] Registered/Logged in as " << username << std::endl;
  } else {
    // Maybe user exists, try login purely? No, rand should be unique enough for
    // this test
    std::cout
        << "[Sender] Registration failed (non-critical if logic handles it)"
        << std::endl;
  }

  // Send Message to Sergey
  wizz::Packet direct(wizz::PacketType::DirectMessage);
  direct.writeString("Sergey");        // Target
  direct.writeString("Hello Sergey!"); // Body
  send_packet(sock, direct);
  std::cout << "[Sender] Parameters sent." << std::endl;

  close_socket(sock);
}

int main() {
#ifdef _WIN32
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

  std::cout << "=== Messaging Test (2 Threads) ===" << std::endl;

  std::thread t1(receiver_thread);
  std::thread t2(sender_thread);

  t1.join();
  t2.join();

#ifdef _WIN32
  WSACleanup();
#endif

  if (g_testPassed) {
    std::cout << "=== TEST PASSED: Message Routed Successfully ==="
              << std::endl;
    return 0;
  } else {
    std::cerr << "=== TEST FAILED: Message not received ===" << std::endl;
    return 1;
  }
}
