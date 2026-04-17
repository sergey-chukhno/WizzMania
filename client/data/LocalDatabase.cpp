#include "LocalDatabase.h"
#include <iostream>

namespace wizz {
namespace client {

LocalDatabase::LocalDatabase(const std::string& dbPath) : m_path(dbPath) {}

LocalDatabase::~LocalDatabase() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) sqlite3_close(m_db);
}

bool LocalDatabase::init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (sqlite3_open(m_path.c_str(), &m_db) != SQLITE_OK) {
        std::cerr << "[ClientDB] Failed to open SQLite DB: " << m_path << std::endl;
        return false;
    }

    const char* sql = 
        "CREATE TABLE IF NOT EXISTS identity ("
        "  ID INTEGER PRIMARY KEY CHECK(ID = 1),"
        "  REG_ID INTEGER NOT NULL,"
        "  PUB_KEY BLOB NOT NULL,"
        "  PRIV_KEY BLOB NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  ADDRESS TEXT PRIMARY KEY,"
        "  RECORD BLOB NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS pre_keys ("
        "  KEY_ID INTEGER PRIMARY KEY,"
        "  RECORD BLOB NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS signed_pre_keys ("
        "  KEY_ID INTEGER PRIMARY KEY,"
        "  RECORD BLOB NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS remote_identities ("
        "  ADDRESS TEXT PRIMARY KEY,"
        "  KEY_BLOB BLOB NOT NULL"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, sql, nullptr, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "[ClientDB] Init Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool LocalDatabase::setIdentity(const std::vector<uint8_t>& pubKey, const std::vector<uint8_t>& privKey, uint32_t registrationId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "INSERT OR REPLACE INTO identity (ID, REG_ID, PUB_KEY, PRIV_KEY) VALUES (1, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_int(stmt, 1, registrationId);
    sqlite3_bind_blob(stmt, 2, pubKey.data(), pubKey.size(), SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, privKey.data(), privKey.size(), SQLITE_STATIC);
    
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::getIdentity(std::vector<uint8_t>& pubKey, std::vector<uint8_t>& privKey, uint32_t& registrationId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT REG_ID, PUB_KEY, PRIV_KEY FROM identity WHERE ID = 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        registrationId = (uint32_t)sqlite3_column_int(stmt, 0);
        
        int pubBytes = sqlite3_column_bytes(stmt, 1);
        const uint8_t* pubBlob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 1));
        pubKey.assign(pubBlob, pubBlob + pubBytes);

        int privBytes = sqlite3_column_bytes(stmt, 2);
        const uint8_t* privBlob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 2));
        privKey.assign(privBlob, privBlob + privBytes);
        
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::storeRemoteIdentity(const std::string& remoteAddress, const std::vector<uint8_t>& identityKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "INSERT OR REPLACE INTO remote_identities (ADDRESS, KEY_BLOB) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, identityKey.data(), (int)identityKey.size(), SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok) std::cerr << "[ClientDB] Stored remote identity for: [" << remoteAddress << "]" << std::endl;
    return ok;
}

bool LocalDatabase::loadRemoteIdentity(const std::string& remoteAddress, std::vector<uint8_t>& identityKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT KEY_BLOB FROM remote_identities WHERE ADDRESS = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int bytes = sqlite3_column_bytes(stmt, 0);
        const uint8_t* blob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 0));
        identityKey.assign(blob, blob + bytes);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::deleteRemoteIdentity(const std::string& remoteAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "DELETE FROM remote_identities WHERE ADDRESS = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok) std::cerr << "[ClientDB] Deleted remote identity for: [" << remoteAddress << "]" << std::endl;
    return ok;
}

bool LocalDatabase::storeSession(const std::string& remoteAddress, const std::vector<uint8_t>& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "INSERT OR REPLACE INTO sessions (ADDRESS, RECORD) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, record.data(), record.size(), SQLITE_STATIC);
    
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE); if (ok) std::cerr << "[ClientDB] Stored session for: " << remoteAddress << " (Len: " << record.size() << ")\n"; else std::cerr << "[ClientDB] FAILED to store session for: " << remoteAddress << "\n";
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::loadSession(const std::string& remoteAddress, std::vector<uint8_t>& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT RECORD FROM sessions WHERE ADDRESS = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int bytes = sqlite3_column_bytes(stmt, 0);
        const uint8_t* blob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 0));
        record.assign(blob, blob + bytes);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::containsSession(const std::string& remoteAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT 1 FROM sessions WHERE ADDRESS = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW); std::cerr << "[ClientDB] containsSession(" << remoteAddress << ") -> " << (found ? "TRUE" : "FALSE") << "\n";
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::deleteSession(const std::string& remoteAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "DELETE FROM sessions WHERE ADDRESS = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, remoteAddress.c_str(), -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::storePreKey(uint32_t preKeyId, const std::vector<uint8_t>& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "INSERT OR REPLACE INTO pre_keys (KEY_ID, RECORD) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, preKeyId);
    sqlite3_bind_blob(stmt, 2, record.data(), record.size(), SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::loadPreKey(uint32_t preKeyId, std::vector<uint8_t>& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT RECORD FROM pre_keys WHERE KEY_ID = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, preKeyId);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int bytes = sqlite3_column_bytes(stmt, 0);
        const uint8_t* blob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 0));
        record.assign(blob, blob + bytes);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::containsPreKey(uint32_t preKeyId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT 1 FROM pre_keys WHERE KEY_ID = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, preKeyId);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::removePreKey(uint32_t preKeyId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "DELETE FROM pre_keys WHERE KEY_ID = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, preKeyId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::storeSignedPreKey(uint32_t signedPreKeyId, const std::vector<uint8_t>& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "INSERT OR REPLACE INTO signed_pre_keys (KEY_ID, RECORD) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, signedPreKeyId);
    sqlite3_bind_blob(stmt, 2, record.data(), record.size(), SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::loadSignedPreKey(uint32_t signedPreKeyId, std::vector<uint8_t>& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT RECORD FROM signed_pre_keys WHERE KEY_ID = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, signedPreKeyId);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int bytes = sqlite3_column_bytes(stmt, 0);
        const uint8_t* blob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, 0));
        record.assign(blob, blob + bytes);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::containsSignedPreKey(uint32_t signedPreKeyId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "SELECT 1 FROM signed_pre_keys WHERE KEY_ID = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, signedPreKeyId);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

bool LocalDatabase::removeSignedPreKey(uint32_t signedPreKeyId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = "DELETE FROM signed_pre_keys WHERE KEY_ID = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, signedPreKeyId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalDatabase::deleteAllData() {
    std::lock_guard<std::mutex> lock(m_mutex);
    const char* sql = 
        "DELETE FROM identity;"
        "DELETE FROM sessions;"
        "DELETE FROM pre_keys;"
        "DELETE FROM signed_pre_keys;";
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, sql, nullptr, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "[ClientDB] Wipe Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

} // namespace client
} // namespace wizz
