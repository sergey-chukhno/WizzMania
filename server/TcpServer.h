#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../common/Types.h"

#include <string>
#include <unordered_map>

#include "../common/Types.h"
#include "ClientSession.h"
#include "DatabaseManager.h"

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

  // Database (The Single Source of Truth)
  DatabaseManager m_db;

  // Day 3: Session Management
  // Key: Socket Descriptor (int), Value: Session Object
  std::unordered_map<SocketType, ClientSession> m_sessions;

  // Day 4: Online Registry (The "Phonebook")
  // Key: Username, Value: Pointer to Session
  std::unordered_map<std::string, ClientSession *> m_onlineUsers;
  // Track Status (1=Online, 2=Busy, 3=Offline)
  std::unordered_map<std::string, int> m_userStatuses;

  // Helpers
  void initSocket();   // Step 1: Create
  void bindSocket();   // Step 2: Bind
  void listenSocket(); // Step 3: Listen
  void cleanup();      // Close and cleanup
};

} // namespace wizz
