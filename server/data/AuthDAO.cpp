#include "AuthDAO.h"
#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace wizz {

static std::string bytesToHex(const unsigned char *bytes, size_t len) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < len; ++i) {
    ss << std::setw(2) << static_cast<int>(bytes[i]);
  }
  return ss.str();
}

AuthDAO::AuthDAO(sqlite3* db) : m_db(db) {}

std::string AuthDAO::generateSalt() {
  const int SALT_LEN = 16;
  unsigned char salt[SALT_LEN];
  if (RAND_bytes(salt, SALT_LEN) != 1) {
    std::cerr << "[AuthDAO] Error generating random salt." << std::endl;
    return "";
  }
  return bytesToHex(salt, SALT_LEN);
}

std::string AuthDAO::hashPassword(const std::string &password, const std::string &salt) {
  std::string combined = password + salt;

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  const EVP_MD *md = EVP_sha256();
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int md_len;

  if (mdctx == nullptr) return "";

  EVP_DigestInit_ex(mdctx, md, nullptr);
  EVP_DigestUpdate(mdctx, combined.c_str(), combined.size());
  EVP_DigestFinal_ex(mdctx, hash, &md_len);
  EVP_MD_CTX_free(mdctx);

  return bytesToHex(hash, md_len);
}

bool AuthDAO::createUser(const std::string &username, const std::string &password) {
  if (!m_db) return false;

  const char *checkSql = "SELECT ID FROM users WHERE LOWER(USERNAME) = LOWER(?);";
  sqlite3_stmt *checkStmt;
  if (sqlite3_prepare_v2(m_db, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(checkStmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
      sqlite3_finalize(checkStmt);
      std::cerr << "[AuthDAO] Insert failed: User already exists." << std::endl;
      return false;
    }
    sqlite3_finalize(checkStmt);
  }

  std::string salt = generateSalt();
  std::string hash = hashPassword(password, salt);

  const char *sql = "INSERT INTO users (USERNAME, PASSWORD_HASH, SALT, AVATAR_PATH) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[AuthDAO] Prepare failed: " << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, "", -1, SQLITE_STATIC);

  bool success = false;
  if (sqlite3_step(stmt) == SQLITE_DONE) {
    success = true;
    std::cout << "[AuthDAO] User Created: " << username << std::endl;
  } else {
    std::cerr << "[AuthDAO] Insert failed: " << sqlite3_errmsg(m_db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return success;
}

bool AuthDAO::checkCredentials(const std::string &username, const std::string &password) {
  if (!m_db) return false;

  const char *sql = "SELECT PASSWORD_HASH, SALT FROM users WHERE LOWER(USERNAME) = LOWER(?);";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

  bool valid = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *storedHash = sqlite3_column_text(stmt, 0);
    const unsigned char *storedSalt = sqlite3_column_text(stmt, 1);

    std::string sHash = reinterpret_cast<const char *>(storedHash);
    std::string sSalt = reinterpret_cast<const char *>(storedSalt);

    if (hashPassword(password, sSalt) == sHash) {
      valid = true;
    }
  }

  sqlite3_finalize(stmt);
  return valid;
}

} // namespace wizz
