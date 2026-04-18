#include "CryptoDAO.h"
#include <sqlite3.h>
#include <iostream>
#include <vector>

namespace wizz {

CryptoDAO::CryptoDAO(sqlite3* db) : m_db(db) {}

bool CryptoDAO::storeUserKeys(const std::string &username, const std::string &identityKey,
                              const std::string &signedPreKey, const std::string &signature, int registrationId) {
  const char *sql = "INSERT INTO public_keys (USERNAME, IDENTITY_KEY, "
                    "SIGNED_PRE_KEY, SIGNED_PRE_KEY_SIG, REGISTRATION_ID) VALUES (?, ?, ?, ?, ?) "
                    "ON CONFLICT(USERNAME) DO UPDATE SET "
                    "IDENTITY_KEY=excluded.IDENTITY_KEY, "
                    "SIGNED_PRE_KEY=excluded.SIGNED_PRE_KEY, "
                    "SIGNED_PRE_KEY_SIG=excluded.SIGNED_PRE_KEY_SIG, "
                    "REGISTRATION_ID=excluded.REGISTRATION_ID;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 2, identityKey.data(), (int)identityKey.size(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, signedPreKey.data(), (int)signedPreKey.size(), SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 4, signature.data(), (int)signature.size(), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 5, registrationId);

  bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
  sqlite3_finalize(stmt);
  return ok;
}

bool CryptoDAO::storeOneTimeKeys(const std::string &username, const std::vector<std::pair<int, std::string>> &otks) {
  if (otks.empty()) return true;

  sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, 0, nullptr);

  const char *sql = "INSERT INTO one_time_keys (USERNAME, KEY_ID, KEY_PUB) VALUES (?, ?, ?);";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, 0, nullptr);
    return false;
  }

  bool success = true;
  for (const auto& otk : otks) {
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, otk.first);
    sqlite3_bind_blob(stmt, 3, otk.second.data(), (int)otk.second.size(), SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
      success = false;
      break;
    }
    sqlite3_reset(stmt);
  }

  sqlite3_finalize(stmt);

  if (success) {
    sqlite3_exec(m_db, "COMMIT;", nullptr, 0, nullptr);
  } else {
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, 0, nullptr);
  }

  return success;
}

bool CryptoDAO::clearUserKeys(const std::string &username) {
  const char *sql1 = "DELETE FROM public_keys WHERE USERNAME = ?;";
  const char *sql2 = "DELETE FROM one_time_keys WHERE USERNAME = ?;";
  
  sqlite3_stmt *stmt1, *stmt2;
  bool ok = true;

  if (sqlite3_prepare_v2(m_db, sql1, -1, &stmt1, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt1, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt1) != SQLITE_DONE) ok = false;
    sqlite3_finalize(stmt1);
  } else ok = false;

  if (sqlite3_prepare_v2(m_db, sql2, -1, &stmt2, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt2, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt2) != SQLITE_DONE) ok = false;
    sqlite3_finalize(stmt2);
  } else ok = false;

  if (ok) std::cout << "[CryptoDAO] Cleared stale keys for " << username << std::endl;
  return ok;
}

PreKeyBundle CryptoDAO::fetchPreKeyBundle(const std::string &username) {
  PreKeyBundle bundle;
  bundle.oneTimeKeyId = -1;
  bundle.registrationId = 0;
  const char *sqlIdentity = "SELECT IDENTITY_KEY, SIGNED_PRE_KEY, SIGNED_PRE_KEY_SIG, REGISTRATION_ID "
                            "FROM public_keys WHERE USERNAME = ?;";
  sqlite3_stmt *stmtIdentity;
  if (sqlite3_prepare_v2(m_db, sqlIdentity, -1, &stmtIdentity, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmtIdentity, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmtIdentity) == SQLITE_ROW) {
      const void* idBlob = sqlite3_column_blob(stmtIdentity, 0);
      int idLen = sqlite3_column_bytes(stmtIdentity, 0);
      bundle.identityKey = std::string(reinterpret_cast<const char*>(idBlob), idLen);

      const void* spkBlob = sqlite3_column_blob(stmtIdentity, 1);
      int spkLen = sqlite3_column_bytes(stmtIdentity, 1);
      bundle.signedPreKey = std::string(reinterpret_cast<const char*>(spkBlob), spkLen);

      const void* sigBlob = sqlite3_column_blob(stmtIdentity, 2);
      int sigLen = sqlite3_column_bytes(stmtIdentity, 2);
      bundle.signedPreKeySignature = std::string(reinterpret_cast<const char*>(sigBlob), sigLen);

      bundle.registrationId = sqlite3_column_int(stmtIdentity, 3);
    }
    sqlite3_finalize(stmtIdentity);
  }

  if (bundle.identityKey.empty()) return bundle;

  const char *sqlPopOtk = "SELECT ID, KEY_ID, KEY_PUB FROM one_time_keys "
                          "WHERE USERNAME = ? ORDER BY ID ASC LIMIT 1;";
  sqlite3_stmt *stmtOtk;
  int primaryIdToDelete = -1;

  if (sqlite3_prepare_v2(m_db, sqlPopOtk, -1, &stmtOtk, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmtOtk, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmtOtk) == SQLITE_ROW) {
      primaryIdToDelete = sqlite3_column_int(stmtOtk, 0);
      bundle.oneTimeKeyId = sqlite3_column_int(stmtOtk, 1);
      const void* otkBlob = sqlite3_column_blob(stmtOtk, 2);
      int otkLen = sqlite3_column_bytes(stmtOtk, 2);
      bundle.oneTimeKey = std::string(reinterpret_cast<const char*>(otkBlob), otkLen);
    }
    sqlite3_finalize(stmtOtk);
  }

  if (primaryIdToDelete != -1) {
    const char *sqlDelete = "DELETE FROM one_time_keys WHERE ID = ?;";
    sqlite3_stmt *stmtDel;
    if (sqlite3_prepare_v2(m_db, sqlDelete, -1, &stmtDel, nullptr) == SQLITE_OK) {
      sqlite3_bind_int(stmtDel, 1, primaryIdToDelete);
      sqlite3_step(stmtDel);
      sqlite3_finalize(stmtDel);
    }
  }

  return bundle;
}

} // namespace wizz
