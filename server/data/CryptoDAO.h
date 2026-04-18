#pragma once

#include "DatabaseTypes.h"
#include <sqlite3.h>
#include <string>
#include <vector>

namespace wizz {

class CryptoDAO {
public:
    explicit CryptoDAO(sqlite3* db);

    bool storeUserKeys(const std::string &username, const std::string &identityKey,
                       const std::string &signedPreKey, const std::string &signature, int registrationId);
    bool storeOneTimeKeys(const std::string &username, const std::vector<std::pair<int, std::string>> &otks);
    bool clearUserKeys(const std::string &username);
    PreKeyBundle fetchPreKeyBundle(const std::string &username);

private:
    sqlite3* m_db;
};

} // namespace wizz
