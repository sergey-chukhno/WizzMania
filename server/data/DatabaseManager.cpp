#include "DatabaseManager.h"
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace wizz {

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
    std::cerr << "[DB] Can't open database: " << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  std::cout << "[DB] Opened successfully: " << m_dbPath << std::endl;

  // Initialize DAOs
  m_authDAO = std::make_unique<AuthDAO>(m_db);
  m_socialDAO = std::make_unique<SocialDAO>(m_db);
  m_cryptoDAO = std::make_unique<CryptoDAO>(m_db);

  // Start the worker thread
  m_workerThread = std::thread(&DatabaseManager::workerLoop, this);

  // Initial Schema Setup
  const char *sqlUsers = "CREATE TABLE IF NOT EXISTS users ("
                         "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "USERNAME TEXT NOT NULL UNIQUE,"
                         "PASSWORD_HASH TEXT NOT NULL,"
                         "SALT TEXT NOT NULL,"
                         "AVATAR_PATH TEXT,"
                         "CUSTOM_STATUS TEXT DEFAULT '');";

  char *errMsg = nullptr;
  if (sqlite3_exec(m_db, sqlUsers, nullptr, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "[DB] Users Table Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  sqlite3_exec(m_db, "ALTER TABLE users ADD COLUMN CUSTOM_STATUS TEXT DEFAULT '';", nullptr, 0, nullptr);

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

  // Seed Default User
  m_authDAO->createUser("Sergey", "Password123!");

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

  const char *sqlPublicKeys = "CREATE TABLE IF NOT EXISTS public_keys ("
                              "USERNAME TEXT PRIMARY KEY,"
                              "IDENTITY_KEY TEXT NOT NULL,"
                              "SIGNED_PRE_KEY TEXT NOT NULL,"
                              "SIGNED_PRE_KEY_SIG TEXT NOT NULL,"
                              "REGISTRATION_ID INTEGER DEFAULT 0,"
                              "FOREIGN KEY(USERNAME) REFERENCES users(USERNAME)"
                              ");";
  if (sqlite3_exec(m_db, sqlPublicKeys, nullptr, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "[DB] Public Keys Table Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  const char *sqlOneTimeKeys = "CREATE TABLE IF NOT EXISTS one_time_keys ("
                               "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "USERNAME TEXT NOT NULL,"
                               "KEY_ID INTEGER NOT NULL,"
                               "KEY_PUB TEXT NOT NULL,"
                               "FOREIGN KEY(USERNAME) REFERENCES users(USERNAME)"
                               ");";
  if (sqlite3_exec(m_db, sqlOneTimeKeys, nullptr, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "[DB] One Time Keys Table Error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  return true;
}

} // namespace wizz
