#include "../../common/Packet.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace wizz;

// --- Test Client Helper ---
struct TestClient {
  int sockfd;
  std::vector<uint8_t> buffer;

  TestClient() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("Socket creation failed");
      exit(1);
    }
  }

  ~TestClient() {
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
  }

  void connect_to_server(int port) {
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("Connection failed");
      exit(1);
    }
  }

  void send_packet(const Packet &pkt) {
    std::vector<uint8_t> data = pkt.serialize();
    send(sockfd, reinterpret_cast<const char *>(data.data()), data.size(), 0);
  }

  Packet receive_packet() {
    // 1. Read Header (12 bytes)
    while (buffer.size() < 12) {
      char temp[1024];
      int n = recv(sockfd, temp, sizeof(temp), 0);
      if (n <= 0)
        throw std::runtime_error("Disconnected or Error");
      buffer.insert(buffer.end(), temp, temp + n);
    }

    // 2. Parse Length
    uint32_t netLen;
    std::memcpy(&netLen, buffer.data() + 8, 4);
    uint32_t bodyLen = ntohl(netLen);

    // 3. Read Body
    while (buffer.size() < 12 + bodyLen) {
      char temp[1024];
      int n = recv(sockfd, temp, sizeof(temp), 0);
      if (n <= 0)
        throw std::runtime_error("Disconnected waiting for body");
      buffer.insert(buffer.end(), temp, temp + n);
    }

    // 4. Construct Packet
    std::vector<uint8_t> pktData(buffer.begin(), buffer.begin() + 12 + bodyLen);
    Packet pkt(pktData);

    // 5. Erase from buffer
    buffer.erase(buffer.begin(), buffer.begin() + 12 + bodyLen);
    return pkt;
  }
};

void login_or_register(TestClient &client, const std::string &username) {
  Packet p(PacketType::Register);
  p.writeString(username);
  p.writeString("pass123");
  client.send_packet(p);

  // Expect LoginSuccess (102) OR RegisterFailed (104) + LoginSuccess (102)
  // Logic: sending Register for existing user returns RegisterFailed.
  // Then we must Login.
  // Wait, offline_test logic was: Try Register -> If fail, Login.

  Packet resp = client.receive_packet();
  if (resp.type() == PacketType::LoginSuccess) {
    std::cout << "[Test] Registered & Logged in as " << username << std::endl;
    // Consume ContactList if present?
    // Wait, LoginSuccess is followed by ContactList now.
    // We should check if there is a next packet.
  } else if (resp.type() == PacketType::RegisterFailed) {
    // Login
    Packet login(PacketType::Login);
    login.writeString(username);
    login.writeString("pass123");
    client.send_packet(login);

    resp = client.receive_packet();
    if (resp.type() != PacketType::LoginSuccess) {
      std::cerr << "Login Failed for " << username << std::endl;
      exit(1);
    }
    std::cout << "[Test] Logged in as " << username << std::endl;
  } else {
    std::cerr << "Unexpected Packet: " << (int)resp.type() << std::endl;
    exit(1);
  }

  // Consume any ContactList packet that might be flushed immediately
  // But we can't block indefinitely if list is empty.
  // For the test, we know if we expect one.
}

int main() {
  std::cout << "=== Contact Management Test ===" << std::endl;

  // 1. Setup Clients
  TestClient alice;
  alice.connect_to_server(8080);
  login_or_register(alice, "AliceContact"); // Will flush empty list? Maybe/No.

  TestClient bob;
  bob.connect_to_server(8080);
  login_or_register(bob, "BobContact");

  // Consume any initial noise
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  alice.buffer.clear(); // Safe-ish for test

  // 2. Alice adds Bob
  std::cout << "[Test] Alice adding Bob..." << std::endl;
  Packet add(PacketType::AddContact);
  add.writeString("BobContact");
  alice.send_packet(add);

  // 3. Expect ContactList with Bob
  Packet resp = alice.receive_packet();
  if (resp.type() == PacketType::ContactList) {
    int count = resp.readInt(); // Size
    std::cout << "[Test] Received Contact List. Size: " << count << std::endl;
    bool found = false;
    for (int i = 0; i < count; ++i) {
      std::string name = resp.readString();
      std::cout << " - " << name << std::endl;
      if (name == "BobContact")
        found = true;
    }

    if (found)
      std::cout << "[PASS] Bob is in the list." << std::endl;
    else {
      std::cerr << "[FAIL] Bob missing from list." << std::endl;
      return 1;
    }
  } else {
    std::cerr << "[FAIL] Expected ContactList, got " << (int)resp.type()
              << std::endl;
    return 1;
  }

  // 4. Login Check (Persistence)
  std::cout << "[Test] Re-connecting Alice to check persistence..."
            << std::endl;
  TestClient alice2;
  alice2.connect_to_server(8080);

  Packet login(PacketType::Login);
  login.writeString("AliceContact");
  login.writeString("pass123");
  alice2.send_packet(login);

  // Expect LoginSuccess
  Packet r1 = alice2.receive_packet();
  if (r1.type() != PacketType::LoginSuccess) {
    std::cerr << "Login failed";
    return 1;
  }

  // Expect ContactList (Auto-pushed)
  Packet r2 = alice2.receive_packet();
  if (r2.type() == PacketType::ContactList) {
    int count = r2.readInt();
    if (count >= 1)
      std::cout << "[PASS] Contact List persisted and synced on login."
                << std::endl;
    else {
      std::cerr << "[FAIL] List empty on re-login." << std::endl;
      return 1;
    }
  } else {
    std::cerr << "[FAIL] Did not receive ContactList on Login. Got: "
              << (int)r2.type() << std::endl;
    // It might be that offline messages came first?
    // In this test, no offline messages.
    return 1;
  }

  // 5. Remove Friend
  std::cout << "[Test] Alice removing Bob..." << std::endl;
  Packet rem(PacketType::RemoveContact);
  rem.writeString("BobContact");
  alice2.send_packet(rem);

  // Expect Updated List (Empty or smaller)
  Packet r3 = alice2.receive_packet();
  if (r3.type() == PacketType::ContactList) {
    int count = r3.readInt();
    std::cout << "[Test] Received Contact List. Size: " << count << std::endl;
    bool found = false;
    for (int i = 0; i < count; ++i) {
      if (r3.readString() == "BobContact")
        found = true;
    }
    if (!found)
      std::cout << "[PASS] Bob removed from list." << std::endl;
    else {
      std::cerr << "[FAIL] Bob still in list." << std::endl;
      return 1;
    }
  }

  std::cout << "=== TEST PASSED ===" << std::endl;
  return 0;
}
