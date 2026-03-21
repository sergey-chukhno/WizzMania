#pragma once

#include <mutex>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>

#include <asio.hpp>
#include <asio/ssl.hpp>

#include "../common/Types.h"
#include "ClientSession.h"
#include "handlers/PacketRouter.h"
#include "DatabaseManager.h"
#include "SessionManager.h"
#include "GameRoomManager.h"

namespace wizz {

class TcpServer {
public:
  explicit TcpServer(int port);
  ~TcpServer();

  // Prevent copying
  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  // Initialize, Bind, Listen, and Run the loop
  void start();
  void run();
  void stop();

  // Thread-safe Response Queue for passing work from DB Thread back to Main Thread
  template <typename F> void postResponse(F &&responseTask) {
    asio::post(m_ioContext, std::forward<F>(responseTask));
  }

  // Safe lookup for async callbacks using Session ID
  ClientSession *getSession(int sessionId);
  void handleDisconnect(int sessionId);

  DatabaseManager &getDb() { return m_db; }
  SessionManager &getSessionManager() { return m_sessionManager; }
  GameRoomManager &getGameRoomManager() { return m_gameRoomManager; }
  PacketRouter &getPacketRouter() { return m_packetRouter; }

private:
  // Boost.Asio Core
  asio::io_context m_ioContext;
  asio::ssl::context m_sslContext;
  asio::ip::tcp::acceptor m_acceptor;

  int m_port;
  bool m_isRunning;
  int m_nextSessionId = 1;

  std::mutex m_responseMutex;
  std::vector<std::function<void()>> m_responses;

  // Database
  DatabaseManager m_db;

  // Core Component Managers
  SessionManager m_sessionManager;
  GameRoomManager m_gameRoomManager;
  PacketRouter m_packetRouter;

  // Asio Accept Loop
  void doAccept();

  void cleanup();
  void setupVoiceStorage();
};

} // namespace wizz
