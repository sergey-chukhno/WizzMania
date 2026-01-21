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

// Reuse Helpers (In a real project, these would be in a test_common.h)
void close_socket_test(SOCKET s) {
#ifdef _WIN32
  closesocket(s);
#else
  close(s);
#endif
}

SOCKET connect_server_test() {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET)
    return INVALID_SOCKET;

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(8080);
  if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
    return INVALID_SOCKET;

  if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    close_socket_test(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

// Robust Packet Reader that maintains a buffer across calls
struct TestClient {
  SOCKET sock;
  std::vector<uint8_t> buffer;

  TestClient(SOCKET s) : sock(s) {}

  bool recv_packet(wizz::PacketType expectedType, std::string *out1 = nullptr,
                   std::string *out2 = nullptr) {
    // 1. Try to parse from existing buffer
    while (true) {
      // Check if we have enough for header
      if (buffer.size() >= 12) {
        // Parse Body Length
        uint32_t netLen;
        std::memcpy(&netLen, buffer.data() + 8, 4);
        uint32_t bodyLen = ntohl(netLen);

        if (buffer.size() >= 12 + bodyLen) {
          // We have a full packet! Extract it.
          std::vector<uint8_t> pktData(buffer.begin(),
                                       buffer.begin() + 12 + bodyLen);

          try {
            wizz::Packet p(pktData);
            // Consume from buffer
            buffer.erase(buffer.begin(), buffer.begin() + 12 + bodyLen);

            if (p.type() == expectedType) {
              if (out1)
                *out1 = p.readString();
              if (out2)
                *out2 = p.readString();
              return true;
            } else {
              std::cout << "[Test] Unexpected Packet: " << (int)p.type()
                        << std::endl;
              // If we see unexpected packet, we return false, but data is
              // consumed. This matches recv_pkt_test semantics but safer.
              return false;
            }
          } catch (...) {
            std::cerr << "[Test] Parse Error" << std::endl;
            return false;
          }
        }
      }

      // 2. Need more data
      char temp[1024];
      int bytes = recv(sock, temp, sizeof(temp), 0);
      if (bytes <= 0)
        return false;
      buffer.insert(buffer.end(), temp, temp + bytes);
    }
  }

  void send_packet(const wizz::Packet &p) {
    std::vector<uint8_t> buf = p.serialize();
    send(sock, reinterpret_cast<const char *>(buf.data()), buf.size(), 0);
  }
};

// Global Sync
std::atomic<bool> g_msgSent{false};
std::atomic<bool> g_passed{false};

// Helper to Login or Register w/ TestClient
bool login_or_register(TestClient &client, const std::string &username) {
  // 1. Try Login
  wizz::Packet login(wizz::PacketType::Login);
  login.writeString(username);
  login.writeString("pass");
  client.send_packet(login);

  if (client.recv_packet(wizz::PacketType::LoginSuccess)) {
    std::cout << "[Test] Logged in as " << username << std::endl;
    return true;
  }

  // 2. Register
  wizz::Packet reg(wizz::PacketType::Register);
  reg.writeString(username);
  reg.writeString("pass");
  client.send_packet(reg);

  // Check next packet (Could be Success or Failed if taken)
  // NOTE: We cannot simply use recv_packet(LoginSuccess) because we might get
  // RegisterFailed. We need a helper that peeks or returns the type. For
  // simplicity, let's assume Register works or fails. Since we can't easily
  // peek with this helper, let's assume if it fails it's because it's taken.

  // Simplification: Just expect LoginSuccess. If it fails, try re-login.
  if (client.recv_packet(wizz::PacketType::LoginSuccess)) {
    std::cout << "[Test] Registered & Logged in as " << username << std::endl;
    return true;
  }

  // If we didn't get LoginSuccess, maybe we got RegisterFailed?
  // Since recv_packet consumes on mismatch, we are in trouble if we don't
  // handle it. But keeping it simple: The previous logic relied on "Try
  // Register, if fail Try Login". With TestClient, we should just retry login.

  std::cout << "[Test] Register path failed/consumed. Retrying Login..."
            << std::endl;
  wizz::Packet login2(wizz::PacketType::Login);
  login2.writeString(username);
  login2.writeString("pass");
  client.send_packet(login2);
  return client.recv_packet(wizz::PacketType::LoginSuccess);
}

void offline_sender() {
  SOCKET sock = connect_server_test();
  if (sock == INVALID_SOCKET)
    return;

  TestClient client(sock);

  if (!login_or_register(client, "SenderOffline")) {
    std::cerr << "[Sender] Auth failed." << std::endl;
    close_socket_test(sock);
    return;
  }

  // Send Message
  wizz::Packet msg(wizz::PacketType::DirectMessage);
  msg.writeString("ReceiverOffline");
  msg.writeString("This is an offline message!");
  client.send_packet(msg);

  std::cout << "[Sender] Sent message to offline user." << std::endl;
  g_msgSent = true;

  close_socket_test(sock);
}

void offline_receiver() {
  while (!g_msgSent)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  SOCKET sock = connect_server_test();
  if (sock == INVALID_SOCKET)
    return;

  TestClient client(sock);

  if (!login_or_register(client, "ReceiverOffline")) {
    std::cerr << "[Receiver] Auth failed." << std::endl;
    close_socket_test(sock);
    return;
  }

  // 2. Expect FLUSHED Message
  std::cout << "[Receiver] Waiting for flushed message..." << std::endl;
  std::string sender, body;
  if (client.recv_packet(wizz::PacketType::DirectMessage, &sender, &body)) {
    std::cout << "[Receiver] Got: " << body << " from " << sender << std::endl;
    if (body == "This is an offline message!" && sender == "SenderOffline") {
      g_passed = true;
    }
  } else {
    std::cerr << "[Receiver] Did not receive offline message." << std::endl;
  }

  close_socket_test(sock);
}

int main() {
#ifdef _WIN32
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

  std::cout << "=== Offline Messaging Test ===" << std::endl;
  // Note: We need to register Receiver first somewhere?
  // Actually, in our Register logic, if user exists it fails login.
  // Ideally we assume fresh DB or use random names.
  // For this test, let's use random suffix to avoid "User Taken" errors on
  // re-runs.

  // Actually, simpler: ClientSession::handleRegister auto-logs in.
  // So receiver_thread just registers. Sender sends to that name.

  std::thread t1(offline_sender);
  t1.join(); // Sender finishes completely.

  std::thread t2(offline_receiver);
  t2.join();

#ifdef _WIN32
  WSACleanup();
#endif

  if (g_passed) {
    std::cout << "=== TEST PASSED: Offline Message Delivered ===" << std::endl;
    return 0;
  } else {
    std::cerr << "=== TEST FAILED ===" << std::endl;
    return 1;
  }
}
