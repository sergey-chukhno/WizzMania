#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

namespace wizz {

class DatabaseManager {
public:
  DatabaseManager(const std::string &dbPath);
  ~DatabaseManager();

  // Prevent copy (Single connection ideally, or manage strictly)
  DatabaseManager(const DatabaseManager &) = delete;
  DatabaseManager &operator=(const DatabaseManager &) = delete;

  // Core Logic
  bool init(); // Creates tables if not exist

  // User Management
  bool createUser(const std::string &username, const std::string &password);
  bool checkCredentials(const std::string &username,
                        const std::string &password);

  // Message Persistence
  struct StoredMessage {
    int id;
    std::string sender;
    std::string body;
    std::string timestamp;
  };

  // Stores a message (Offline or History)
  bool storeMessage(const std::string &sender, const std::string &recipient,
                    const std::string &body, bool isDelivered);

  // Retrieves undelivered messages for a user
  std::vector<StoredMessage> fetchPendingMessages(const std::string &recipient);

  // Marks a list of message IDs as delivered
  void markAsDelivered(int msgId);

  // Contact Management (Day 6)
  bool addFriend(const std::string &username, const std::string &friendName);
  bool removeFriend(const std::string &username, const std::string &friendName);
  std::vector<std::string> getFriends(const std::string &username);

private:
  std::string hashPassword(const std::string &password,
                           const std::string &salt);
  std::string generateSalt();

private:
  std::string m_dbPath;
  sqlite3 *m_db;
};

} // namespace wizz
