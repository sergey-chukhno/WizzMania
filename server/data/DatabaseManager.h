#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <string>
#include <thread>
#include <vector>
#include <memory>

#include "DatabaseTypes.h"
#include "AuthDAO.h"
#include "SocialDAO.h"
#include "CryptoDAO.h"

namespace wizz {

class DatabaseManager {
public:
  DatabaseManager(const std::string &dbPath);
  ~DatabaseManager();

  // Actor Model: Enqueue task for background DB thread
  void postTask(std::function<void()> task);

  // Prevent copy
  DatabaseManager(const DatabaseManager &) = delete;
  DatabaseManager &operator=(const DatabaseManager &) = delete;

  // Core Logic
  bool init(); // Creates tables and initializes DAOs

  // Facade Accessors
  AuthDAO* auth() const { return m_authDAO.get(); }
  SocialDAO* social() const { return m_socialDAO.get(); }
  CryptoDAO* crypto() const { return m_cryptoDAO.get(); }

private:
  void workerLoop();

private:
  std::string m_dbPath;
  sqlite3 *m_db;

  // DAOs
  std::unique_ptr<AuthDAO> m_authDAO;
  std::unique_ptr<SocialDAO> m_socialDAO;
  std::unique_ptr<CryptoDAO> m_cryptoDAO;

  // Background Processing
  std::thread m_workerThread;
  std::queue<std::function<void()>> m_tasks;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::atomic<bool> m_stopWorker;
};

} // namespace wizz
