#include "DatabaseManager.h"
#include <iomanip> // Added based on user's snippet
#include <iostream>
#include <mutex>          // Added based on user's snippet
#include <openssl/evp.h>  // Added based on user's instruction
#include <openssl/rand.h> // Added based on user's instruction
#include <openssl/sha.h>  // Added for SHA256
#include <sstream>

namespace wizz {

// Helper function to convert byte array to hex string
std::string bytesToHex(const unsigned char *bytes, size_t len) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < len; ++i) {
    ss << std::setw(2) << static_cast<int>(bytes[i]);
  }
  return ss.str();
}

// New generateSalt implementation using OpenSSL RAND_bytes
std::string DatabaseManager::generateSalt() {
  const int SALT_LEN = 16; // 128-bit salt
  unsigned char salt[SALT_LEN];
  if (RAND_bytes(salt, SALT_LEN) != 1) {
    // Handle error, e.g., log and throw exception or return empty string
    std::cerr << "[DB] Error generating random salt." << std::endl;
    return "";
  }
  return bytesToHex(salt, SALT_LEN);
}

// New hashPassword implementation using OpenSSL EVP API (replacing deprecated
// SHA256_*)
std::string DatabaseManager::hashPassword(const std::string &password,
                                          const std::string &salt) {
  std::string combined = password + salt;

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  const EVP_MD *md = EVP_sha256();
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int md_len;

  if (mdctx == nullptr)
    return "";

  EVP_DigestInit_ex(mdctx, md, nullptr);
  EVP_DigestUpdate(mdctx, combined.c_str(), combined.size());
  EVP_DigestFinal_ex(mdctx, hash, &md_len);
  EVP_MD_CTX_free(mdctx);

  return bytesToHex(hash, md_len);
}

DatabaseManager::DatabaseManager(const std::string &dbPath)
    : m_dbPath(dbPath), m_db(nullptr), m_stopWorker(false) {}

DatabaseManager::~DatabaseManager() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopWorker = true;
  }
  m_cv.notify_one();
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }

  if (m_db) {
    sqlite3_close(m_db);
    std::cout << "[DB] Connection Closed." << std::endl;
  }
}

void DatabaseManager::postTask(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push(std::move(task));
  }
  m_cv.notify_one();
}

void DatabaseManager::workerLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this] { return m_stopWorker || !m_tasks.empty(); });

      if (m_stopWorker && m_tasks.empty()) {
        break;
      }

      task = std::move(m_tasks.front());
      m_tasks.pop();
    }
    if (task) {
      task();
    }
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

  // Start the worker thread
  m_workerThread = std::thread(&DatabaseManager::workerLoop, this);

  // 2. Create Users Table
  const char *sqlUsers = "CREATE TABLE IF NOT EXISTS users ("
                         "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "USERNAME TEXT NOT NULL UNIQUE,"
                         "PASSWORD_HASH TEXT NOT NULL,"
                         "SALT TEXT NOT NULL,"
                         "AVATAR_PATH TEXT);";

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

  // 4. Create Friends Table (Day 6)
  const char *sqlFriends = "CREATE TABLE IF NOT EXISTS friends ("
                           "user_id INTEGER NOT NULL,"
                           "friend_id INTEGER NOT NULL,"
                           "PRIMARY KEY (user_id, friend_id),"
                           "FOREIGN KEY(user_id) REFERENCES users(ID),"
                           "FOREIGN KEY(friend_id) REFERENCES users(ID)"
                           ");";

  if (sqlite3_exec(m_db, sqlFriends, nullptr, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "[DB] Friends Table Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  return true;
}

// --- Contact Management ---

bool DatabaseManager::addFriend(const std::string &username,
                                const std::string &friendName) {
  // 1. Get IDs (Nested Select is easier but let's be explicit for safety)
  // Actually, standard SQL INSERT INTO ... SELECT ... is best.
  // "INSERT OR IGNORE INTO friends (user_id, friend_id)
  //  SELECT u1.id, u2.id FROM users u1, users u2
  //  WHERE u1.USERNAME = ? AND u2.USERNAME = ?"

  const char *sql = "INSERT OR IGNORE INTO friends (user_id, friend_id) "
                    "SELECT u1.ID, u2.ID FROM users u1, users u2 "
                    "WHERE u1.USERNAME = ? AND u2.USERNAME = ?;";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB] Prepare failed (addFriend): " << sqlite3_errmsg(m_db)
              << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, friendName.c_str(), -1, SQLITE_STATIC);

  bool success = false;
  if (sqlite3_step(stmt) == SQLITE_DONE) {
    // Check if we actually inserted anything (meaning both users exist)
    if (sqlite3_changes(m_db) > 0) {
      success = true;
    } else {
      // Either duplicate (which is fine, technically success) or user not
      // found. To distinguish, we'd need more logic. For now, if "INSERT OR
      // IGNORE" did nothing, it might be partial. Let's assume Success if users
      // exist. Actually strictly: The generic requirement is "Error if user
      // does not exist". with this query, if user doesn't exist, changes() ==
      // 0.
      success = false; // We will handle "User Not Found" logic higher up or
                       // check changes.
      // Wait, sqlite3_changes returns 0 if it was IGNORED (already friends).
      // We need to differentiate "Already Friends" (Success) vs "User Missing"
      // (Fail).

      // Let's do a quick check for Friend Existence to be precise.
      // OR: Just keep strict. If changes() == 0, check if already friends?
      // Optimization: For this project, let's keep it simple.
      // We will assume if count is 0, it failed.
      // User requested "Error if user doesn't exist".
    }
  }

  // Revised Logic for precision:
  // 1. Check if friend exists.
  // 2. Insert.
  sqlite3_finalize(stmt);

  if (success)
    return true; // It worked directly.

  // If changes == 0, it could be duplicate. Let's check if friend exists.
  std::string checkSql = "SELECT ID FROM users WHERE USERNAME = ?;";
  sqlite3_stmt *checkStmt;
  sqlite3_prepare_v2(m_db, checkSql.c_str(), -1, &checkStmt, nullptr);
  sqlite3_bind_text(checkStmt, 1, friendName.c_str(), -1, SQLITE_STATIC);
  bool friendExists = (sqlite3_step(checkStmt) == SQLITE_ROW);
  sqlite3_finalize(checkStmt);

  if (!friendExists)
    return false;

  // If friend exists, try insert without ignore to see error, or just assume it
  // was duplicate. If we are here, friend exists. Retry the insert? No,
  // changes()=0 means it was ignored. So return True (Idempotent success).
  return true;
}

bool DatabaseManager::removeFriend(const std::string &username,
                                   const std::string &friendName) {
  const char *sql = "DELETE FROM friends WHERE "
                    "user_id = (SELECT ID FROM users WHERE USERNAME = ?) AND "
                    "friend_id = (SELECT ID FROM users WHERE USERNAME = ?);";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, friendName.c_str(), -1, SQLITE_STATIC);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true; // Always succeed
}

std::vector<std::string>
DatabaseManager::getFollowers(const std::string &username) {
  std::vector<std::string> followers;
  const char *sql =
      "SELECT u.USERNAME FROM users u "
      "JOIN friends f ON u.ID = f.user_id "
      "WHERE f.friend_id = (SELECT ID FROM users WHERE USERNAME = ?);";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return followers;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *name =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (name)
      followers.emplace_back(name);
  }

  sqlite3_finalize(stmt);
  return followers;
}

std::vector<std::string>
DatabaseManager::getFriends(const std::string &username) {
  std::vector<std::string> friends;
  // Get Friend ID -> Join Users
  const char *sql =
      "SELECT u.USERNAME FROM users u "
      "JOIN friends f ON u.ID = f.friend_id "
      "WHERE f.user_id = (SELECT ID FROM users WHERE USERNAME = ?);";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return friends;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *name =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (name)
      friends.emplace_back(name);
  }

  sqlite3_finalize(stmt);
  return friends;
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
  // Prepare Statement (PREVENT SQL INJECTION)
  const char *sql = "INSERT INTO users (USERNAME, PASSWORD_HASH, SALT, "
                    "AVATAR_PATH) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  // Bind Constants
  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, "", -1, SQLITE_STATIC); // Default empty avatar

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

bool DatabaseManager::updateUserAvatar(const std::string &username,
                                       const std::string &avatarPath) {
  const char *sql = "UPDATE users SET AVATAR_PATH = ? WHERE USERNAME = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, avatarPath.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

  bool success = (sqlite3_step(stmt) == SQLITE_DONE);
  sqlite3_finalize(stmt);
  return success;
}

std::string DatabaseManager::getUserAvatar(const std::string &username) {
  const char *sql = "SELECT AVATAR_PATH FROM users WHERE USERNAME = ?;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return "";

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  std::string path;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *text = sqlite3_column_text(stmt, 0);
    if (text)
      path = reinterpret_cast<const char *>(text);
  }
  sqlite3_finalize(stmt);
  return path;
}

} // namespace wizz
