#pragma once

#include <sqlite3.h>
#include <string>

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

private:
  std::string hashPassword(const std::string &password,
                           const std::string &salt);
  std::string generateSalt();

private:
  std::string m_dbPath;
  sqlite3 *m_db;
};

} // namespace wizz
