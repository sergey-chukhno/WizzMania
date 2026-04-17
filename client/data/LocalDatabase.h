#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <sqlite3.h> // Linked natively via CMake

namespace wizz {
namespace client {

/**
 * @class LocalDatabase
 * @brief Thread-safe SQLite wrapper for storing local E2EE keys and sessions natively.
 */
class LocalDatabase {
public:
    LocalDatabase(const std::string& dbPath);
    ~LocalDatabase();

    bool init();
    std::string getDbPath() const { return m_path; }

    // -- Identity & Registration --
    bool setIdentity(const std::vector<uint8_t>& pubKey, const std::vector<uint8_t>& privKey, uint32_t registrationId);
    bool getIdentity(std::vector<uint8_t>& pubKey, std::vector<uint8_t>& privKey, uint32_t& registrationId);
    bool storeRemoteIdentity(const std::string& remoteAddress, const std::vector<uint8_t>& identityKey);
    bool loadRemoteIdentity(const std::string& remoteAddress, std::vector<uint8_t>& identityKey);
    bool deleteRemoteIdentity(const std::string& remoteAddress);

    // -- Sessions --
    bool storeSession(const std::string& remoteAddress, const std::vector<uint8_t>& record);
    bool loadSession(const std::string& remoteAddress, std::vector<uint8_t>& record);
    bool containsSession(const std::string& remoteAddress);
    bool deleteSession(const std::string& remoteAddress);

    // -- PreKeys --
    bool storePreKey(uint32_t preKeyId, const std::vector<uint8_t>& record);
    bool loadPreKey(uint32_t preKeyId, std::vector<uint8_t>& record);
    bool containsPreKey(uint32_t preKeyId);
    bool removePreKey(uint32_t preKeyId);

    // -- Signed PreKeys --
    bool storeSignedPreKey(uint32_t signedPreKeyId, const std::vector<uint8_t>& record);
    bool loadSignedPreKey(uint32_t signedPreKeyId, std::vector<uint8_t>& record);
    bool containsSignedPreKey(uint32_t signedPreKeyId);
    bool removeSignedPreKey(uint32_t signedPreKeyId);
    bool deleteAllData();

private:
    sqlite3* m_db = nullptr;
    std::string m_path;
    std::mutex m_mutex;
};

} // namespace client
} // namespace wizz
