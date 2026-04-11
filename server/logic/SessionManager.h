#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace wizz {

class ClientSession;

class SessionManager {
public:
  SessionManager() = default;
  ~SessionManager() = default;

  // Core Session Tracking
  void addSession(int sessionId, std::shared_ptr<ClientSession> session);
  void removeSession(int sessionId);
  ClientSession* getSessionById(int sessionId) const;

  // Online User Tracking (The "Phonebook")
  void setUserOnline(const std::string& username, ClientSession* session, const std::string& customStatus);
  void setUserOffline(const std::string& username);
  ClientSession* getSessionByUsername(const std::string& username) const;
  bool isUserOnline(const std::string& username) const;

  // Status Management (0=Online, 1=Away, 2=Busy, 3=Offline)
  void updateStatus(const std::string& username, int status);
  int getStatus(const std::string& username) const;

  void updateCustomStatus(const std::string& username, const std::string& customStatus);
  std::string getCustomStatus(const std::string& username) const;

  // Utilities for broadcasting
  std::vector<ClientSession*> getAllOnlineSessions() const;
  std::vector<std::string> getAllOnlineUsernames() const;

private:
  // Prevents shared_ptr lifecycle drops during async I/O
  std::unordered_map<int, std::shared_ptr<ClientSession>> m_sessions;
  
  // Active user mapping
  std::unordered_map<std::string, ClientSession*> m_onlineUsers;
  std::unordered_map<std::string, int> m_userStatuses;
  std::unordered_map<std::string, std::string> m_customStatuses;
};

} // namespace wizz
