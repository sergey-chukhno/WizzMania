#include "SessionManager.h"
#include "../core/ClientSession.h"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace wizz {

static std::string normalize(const std::string& str) {
    std::string data = str;
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return data;
}

void SessionManager::addSession(int sessionId, std::shared_ptr<ClientSession> session) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_sessions[sessionId] = std::move(session);
}

void SessionManager::removeSession(int sessionId) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_sessions.find(sessionId);
  if (it != m_sessions.end()) {
    // If this session was tied to a user, remove them from the phonebook too
    auto username = it->second->getUsername();
    if (!username.empty()) {
        setUserOffline(normalize(username));
    }
    m_sessions.erase(it);
  }
}

ClientSession* SessionManager::getSessionById(int sessionId) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_sessions.find(sessionId);
  return (it != m_sessions.end()) ? it->second.get() : nullptr;
}

size_t SessionManager::getActiveSessionCount() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_sessions.size();
}

void SessionManager::setUserOnline(const std::string& username, ClientSession* session, const std::string& customStatus) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::string normalized = normalize(username);
  m_onlineUsers[normalized] = session;
  m_userStatuses[normalized] = 0; // Default to Online
  m_customStatuses[normalized] = customStatus;
  std::cout << "[SessionManager] User Online: " << username << " (normalized: " << normalized << ") Status: 0" << std::endl;
}

void SessionManager::setUserOffline(const std::string& username) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::string normalized = normalize(username);
  m_onlineUsers.erase(normalized);
  m_userStatuses.erase(normalized);
  m_customStatuses.erase(normalized);
  std::cout << "[SessionManager] User Offline: " << username << " (normalized: " << normalized << ")" << std::endl;
}

ClientSession* SessionManager::getSessionByUsername(const std::string& username) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto it = m_onlineUsers.find(normalize(username));
  return (it != m_onlineUsers.end()) ? it->second : nullptr;
}

bool SessionManager::isUserOnline(const std::string& username) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_onlineUsers.find(normalize(username)) != m_onlineUsers.end();
}

void SessionManager::updateStatus(const std::string& username, int status) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::string normalized = normalize(username);
  if (isUserOnline(normalized)) {
    m_userStatuses[normalized] = status;
  }
}

int SessionManager::getStatus(const std::string& username) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::string normalized = normalize(username);
  auto it = m_userStatuses.find(normalized);
  return (it != m_userStatuses.end()) ? it->second : 3; // 3 = Offline
}

void SessionManager::updateCustomStatus(const std::string& username, const std::string& customStatus) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::string normalized = normalize(username);
  if (isUserOnline(normalized)) {
    m_customStatuses[normalized] = customStatus;
  }
}

std::string SessionManager::getCustomStatus(const std::string& username) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::string normalized = normalize(username);
  auto it = m_customStatuses.find(normalized);
  return (it != m_customStatuses.end()) ? it->second : "";
}

std::vector<ClientSession*> SessionManager::getAllOnlineSessions() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<ClientSession*> sessions;
  sessions.reserve(m_onlineUsers.size());
  for (const auto& [name, session] : m_onlineUsers) {
    sessions.push_back(session);
  }
  return sessions;
}

std::vector<std::string> SessionManager::getAllOnlineUsernames() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<std::string> names;
  names.reserve(m_onlineUsers.size());
  for (const auto& [name, session] : m_onlineUsers) {
    if (session) {
        names.push_back(session->getUsername());
    } else {
        names.push_back(name);
    }
  }
  return names;
}

} // namespace wizz
