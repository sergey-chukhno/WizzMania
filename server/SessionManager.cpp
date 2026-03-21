#include "SessionManager.h"
#include "ClientSession.h"

namespace wizz {

void SessionManager::addSession(int sessionId, std::shared_ptr<ClientSession> session) {
  m_sessions[sessionId] = std::move(session);
}

void SessionManager::removeSession(int sessionId) {
  auto it = m_sessions.find(sessionId);
  if (it != m_sessions.end()) {
    // If this session was tied to a user, remove them from the phonebook too
    auto username = it->second->getUsername();
    if (!username.empty()) {
        setUserOffline(username);
    }
    m_sessions.erase(it);
  }
}

ClientSession* SessionManager::getSessionById(int sessionId) const {
  auto it = m_sessions.find(sessionId);
  return (it != m_sessions.end()) ? it->second.get() : nullptr;
}

void SessionManager::setUserOnline(const std::string& username, ClientSession* session, const std::string& customStatus) {
  m_onlineUsers[username] = session;
  m_userStatuses[username] = 0; // Default to Online
  m_customStatuses[username] = customStatus;
}

void SessionManager::setUserOffline(const std::string& username) {
  m_onlineUsers.erase(username);
  m_userStatuses.erase(username);
  m_customStatuses.erase(username);
}

ClientSession* SessionManager::getSessionByUsername(const std::string& username) const {
  auto it = m_onlineUsers.find(username);
  return (it != m_onlineUsers.end()) ? it->second : nullptr;
}

bool SessionManager::isUserOnline(const std::string& username) const {
  return m_onlineUsers.find(username) != m_onlineUsers.end();
}

void SessionManager::updateStatus(const std::string& username, int status) {
  if (isUserOnline(username)) {
    m_userStatuses[username] = status;
  }
}

int SessionManager::getStatus(const std::string& username) const {
  auto it = m_userStatuses.find(username);
  return (it != m_userStatuses.end()) ? it->second : 3; // 3 = Offline
}

void SessionManager::updateCustomStatus(const std::string& username, const std::string& customStatus) {
  if (isUserOnline(username)) {
    m_customStatuses[username] = customStatus;
  }
}

std::string SessionManager::getCustomStatus(const std::string& username) const {
  auto it = m_customStatuses.find(username);
  return (it != m_customStatuses.end()) ? it->second : "";
}

std::vector<ClientSession*> SessionManager::getAllOnlineSessions() const {
  std::vector<ClientSession*> sessions;
  sessions.reserve(m_onlineUsers.size());
  for (const auto& [name, session] : m_onlineUsers) {
    sessions.push_back(session);
  }
  return sessions;
}

std::vector<std::string> SessionManager::getAllOnlineUsernames() const {
  std::vector<std::string> names;
  names.reserve(m_onlineUsers.size());
  for (const auto& [name, session] : m_onlineUsers) {
    names.push_back(name);
  }
  return names;
}

} // namespace wizz
