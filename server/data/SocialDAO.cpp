#include "SocialDAO.h"
#include <sqlite3.h>
#include <iostream>
#include <vector>

namespace wizz {

SocialDAO::SocialDAO(sqlite3* db) : m_db(db) {}

int SocialDAO::addFriend(const std::string &username, const std::string &friendName) {
  if (username == friendName) return 3;

  const char *sql = "INSERT OR IGNORE INTO friends (user_id, friend_id) "
                    "SELECT u1.ID, u2.ID FROM users u1, users u2 "
                    "WHERE LOWER(u1.USERNAME) = LOWER(?) AND LOWER(u2.USERNAME) = LOWER(?);";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[SocialDAO] Prepare failed (addFriend): " << sqlite3_errmsg(m_db) << std::endl;
    return 3;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, friendName.c_str(), -1, SQLITE_STATIC);

  int status = 3;
  if (sqlite3_step(stmt) == SQLITE_DONE) {
    if (sqlite3_changes(m_db) > 0) {
      status = 0;
    } else {
      std::string checkSql = "SELECT ID FROM users WHERE LOWER(USERNAME) = LOWER(?);";
      sqlite3_stmt *checkStmt;
      sqlite3_prepare_v2(m_db, checkSql.c_str(), -1, &checkStmt, nullptr);
      sqlite3_bind_text(checkStmt, 1, friendName.c_str(), -1, SQLITE_STATIC);
      bool friendExists = (sqlite3_step(checkStmt) == SQLITE_ROW);
      sqlite3_finalize(checkStmt);

      if (!friendExists) {
        status = 1;
      } else {
        status = 2;
      }
    }
  }

  sqlite3_finalize(stmt);
  return status;
}

bool SocialDAO::removeFriend(const std::string &username, const std::string &friendName) {
  const char *sql = "DELETE FROM friends WHERE "
                    "user_id = (SELECT ID FROM users WHERE LOWER(USERNAME) = LOWER(?)) AND "
                    "friend_id = (SELECT ID FROM users WHERE LOWER(USERNAME) = LOWER(?));";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, friendName.c_str(), -1, SQLITE_STATIC);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

std::vector<std::string> SocialDAO::getFollowers(const std::string &username) {
  std::vector<std::string> followers;
  const char *sql = "SELECT u.USERNAME FROM users u "
                    "JOIN friends f ON u.ID = f.user_id "
                    "WHERE f.friend_id = (SELECT ID FROM users WHERE LOWER(USERNAME) = LOWER(?));";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return followers;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (name) followers.emplace_back(name);
  }

  sqlite3_finalize(stmt);
  return followers;
}

std::vector<std::string> SocialDAO::getFriends(const std::string &username) {
  std::vector<std::string> friends;
  const char *sql = "SELECT u.USERNAME FROM users u "
                    "JOIN friends f ON u.ID = f.friend_id "
                    "WHERE f.user_id = (SELECT ID FROM users WHERE LOWER(USERNAME) = LOWER(?));";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return friends;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (name) friends.emplace_back(name);
  }

  sqlite3_finalize(stmt);
  return friends;
}

bool SocialDAO::storeMessage(const std::string &sender, const std::string &recipient,
                             const std::string &body, bool isDelivered) {
  const char *sql = "INSERT INTO messages (sender, recipient, body, is_delivered) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, recipient.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, body.c_str(), body.length(), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 4, isDelivered ? 1 : 0);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

std::vector<StoredMessage> SocialDAO::fetchPendingMessages(const std::string &recipient) {
  std::vector<StoredMessage> messages;
  const char *sql = "SELECT id, sender, body, timestamp FROM messages WHERE "
                    "recipient = ? AND is_delivered = 0 LIMIT 50;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return messages;

  sqlite3_bind_text(stmt, 1, recipient.c_str(), -1, SQLITE_STATIC);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    StoredMessage msg;
    msg.id = sqlite3_column_int(stmt, 0);
    msg.sender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    int bytes = sqlite3_column_bytes(stmt, 2);
    msg.body = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)), bytes);
    messages.push_back(msg);
  }

  sqlite3_finalize(stmt);
  return messages;
}

void SocialDAO::markAsDelivered(int msgId) {
  const char *sql = "UPDATE messages SET is_delivered = 1 WHERE id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, msgId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }
}

bool SocialDAO::updateUserAvatar(const std::string &username, const std::string &avatarPath) {
  const char *sql = "UPDATE users SET AVATAR_PATH = ? WHERE LOWER(USERNAME) = LOWER(?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, avatarPath.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

  bool success = (sqlite3_step(stmt) == SQLITE_DONE);
  sqlite3_finalize(stmt);
  return success;
}

std::string SocialDAO::getUserAvatar(const std::string &username) {
  const char *sql = "SELECT AVATAR_PATH FROM users WHERE LOWER(USERNAME) = LOWER(?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return "";

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  std::string path;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *text = sqlite3_column_text(stmt, 0);
    if (text) path = reinterpret_cast<const char *>(text);
  }
  sqlite3_finalize(stmt);
  return path;
}

bool SocialDAO::updateCustomStatus(const std::string &username, const std::string &status) {
  const char *sql = "UPDATE users SET CUSTOM_STATUS = ? WHERE LOWER(USERNAME) = LOWER(?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);

  bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
  sqlite3_finalize(stmt);
  return ok;
}

std::string SocialDAO::getCustomStatus(const std::string &username) {
  const char *sql = "SELECT CUSTOM_STATUS FROM users WHERE LOWER(USERNAME) = LOWER(?);";
  sqlite3_stmt *stmt;
  std::string status = "";
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const unsigned char *text = sqlite3_column_text(stmt, 0);
      if (text) status = reinterpret_cast<const char *>(text);
    }
    sqlite3_finalize(stmt);
  }
  return status;
}

} // namespace wizz
