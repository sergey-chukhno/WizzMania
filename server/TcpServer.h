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

  // Thread-safe Response Queue for passing work from DB Thread back to Main
  // Thread
  void postResponse(std::function<void()> responseTask);

  // Safe lookup for async callbacks
  ClientSession *getSession(SocketType socket);
  DatabaseManager &getDb() { return m_db; }

private:
  void processResponses();

  int m_port;
  SocketType m_serverSocket;
  bool m_isRunning;

  std::mutex m_responseMutex;
  std::vector<std::function<void()>> m_responses;

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
  void initSocket();        // Step 1: Create
  void bindSocket();        // Step 2: Bind
  void listenSocket();      // Step 3: Listen
  void cleanup();           // Close and cleanup
  void setupVoiceStorage(); // Create storage directory

  // Client Session Handlers (Extracted from run() loop)
  void handleLogin(ClientSession *session);
  void handleMessage(ClientSession *sender, const std::string &target,
                     const std::string &msg);
  void handleNudge(ClientSession *sender, const std::string &target);
  void handleVoiceMessage(ClientSession *sender, const std::string &target,
                          uint16_t duration, const std::vector<uint8_t> &data);
  void handleTypingIndicator(ClientSession *sender, const std::string &target,
                             bool isTyping);
  int handleGetStatus(const std::string &username);
  void handleStatusChange(ClientSession *sender, int newStatus);
  void handleUpdateAvatar(ClientSession *sender,
                          const std::vector<uint8_t> &data);
  void handleGetAvatar(ClientSession *sender, const std::string &target);
};

} // namespace wizz
