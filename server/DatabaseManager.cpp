#include "DatabaseManager.h"
#include <functional> // For std::hash (Mock hashing)
#include <iostream>
#include <sstream>

namespace wizz {

DatabaseManager::DatabaseManager(const std::string &dbPath)
    : m_dbPath(dbPath), m_db(nullptr) {}

DatabaseManager::~DatabaseManager() {
  if (m_db) {
    sqlite3_close(m_db);
    std::cout << "[DB] Connection Closed." << std::endl;
  }
}
bool DatabaseManager::init() {
  // 1. Open Connection
  int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
  if (rc) {
    std::cerr << "[DB] Can't open database: " << sqlite3_errmsg(m_db)
              << std::endl;
    return false;
  }

  std::cout << "[DB] Opened successfully: " << m_dbPath << std::endl;

  // 2. Create Users Table
  const char *sqlUsers = "CREATE TABLE IF NOT EXISTS users ("
                         "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "USERNAME TEXT NOT NULL UNIQUE,"
                         "PASSWORD_HASH TEXT NOT NULL,"
                         "SALT TEXT NOT NULL);";

  char *errMsg = nullptr;
  if (sqlite3_exec(m_db, sqlUsers, nullptr, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "[DB] Users Table Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  // 3. Create Messages Table
  const char *sqlMsgs = "CREATE TABLE IF NOT EXISTS messages ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "sender TEXT NOT NULL,"
                        "recipient TEXT NOT NULL,"
                        "body TEXT NOT NULL,"
                        "timestamp INTEGER DEFAULT (strftime('%s', 'now')),"
                        "is_delivered INTEGER DEFAULT 0"
                        ");";

  if (sqlite3_exec(m_db, sqlMsgs, nullptr, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "[DB] Messages Table Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  // Seed Default User (Dev Mode)
  createUser("Sergey", "Password123!");

  return true;
}

bool DatabaseManager::storeMessage(const std::string &sender,
                                   const std::string &recipient,
                                   const std::string &body, bool isDelivered) {
  const char *sql = "INSERT INTO messages (sender, recipient, body, "
                    "is_delivered) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, recipient.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, body.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 4, isDelivered ? 1 : 0);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "[DB] Msg Insert failed: " << sqlite3_errmsg(m_db)
              << std::endl;
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

std::vector<DatabaseManager::StoredMessage>
DatabaseManager::fetchPendingMessages(const std::string &recipient) {
  std::vector<StoredMessage> messages;
  // LIMIT 50 to prevent freezing the server loop
  const char *sql = "SELECT id, sender, body, timestamp FROM messages WHERE "
                    "recipient = ? AND is_delivered = 0 LIMIT 50;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return messages;
  }

  sqlite3_bind_text(stmt, 1, recipient.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    StoredMessage msg;
    msg.id = sqlite3_column_int(stmt, 0);
    msg.sender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    msg.body = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    messages.push_back(msg);
  }

  sqlite3_finalize(stmt);
  return messages;
}

void DatabaseManager::markAsDelivered(int msgId) {
  const char *sql = "UPDATE messages SET is_delivered = 1 WHERE id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, msgId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }
}

bool DatabaseManager::createUser(const std::string &username,
                                 const std::string &password) {
  if (!m_db)
    return false;

  std::string salt = generateSalt();
  std::string hash = hashPassword(password, salt);

  // Prepare Statement (PREVENT SQL INJECTION)
  const char *sql =
      "INSERT INTO users (USERNAME, PASSWORD_HASH, SALT) VALUES (?, ?, ?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  // Bind Constants
  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);

  bool success = false;
  if (sqlite3_step(stmt) == SQLITE_DONE) {
    success = true;
    std::cout << "[DB] User Created: " << username << std::endl;
  } else {
    std::cerr << "[DB] Insert failed (Duplicate user?): "
              << sqlite3_errmsg(m_db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return success;
}

bool DatabaseManager::checkCredentials(const std::string &username,
                                       const std::string &password) {
  if (!m_db)
    return false;

  const char *sql = "SELECT PASSWORD_HASH, SALT FROM users WHERE USERNAME = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  bool valid = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    // User exists, check password
    const unsigned char *storedHash = sqlite3_column_text(stmt, 0);
    const unsigned char *storedSalt = sqlite3_column_text(stmt, 1);

    std::string sHash = reinterpret_cast<const char *>(storedHash);
    std::string sSalt = reinterpret_cast<const char *>(storedSalt);

    // Re-hash input
    if (hashPassword(password, sSalt) == sHash) {
      valid = true;
    }
  }

  sqlite3_finalize(stmt);
  return valid;
}

// --- Helpers ---
std::string DatabaseManager::generateSalt() {
  // Mock Salt for "Day 3" (In production, use random bytes)
  return "salty_User_";
}

std::string DatabaseManager::hashPassword(const std::string &password,
                                          const std::string &salt) {
  // Mock Hash (std::hash) - NOT CRYPTOGRAPHICALLY SECURE
  // TODO: Replace with SHA256 (OpenSSL) later
  std::string combo = salt + password;
  std::hash<std::string> hasher;
  size_t hashVal = hasher(combo);

  std::stringstream ss;
  ss << std::hex << hashVal;
  return ss.str();
}

} // namespace wizz
