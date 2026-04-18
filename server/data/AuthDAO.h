#pragma once

#include "DatabaseTypes.h"
#include <sqlite3.h>
#include <string>

namespace wizz {

class AuthDAO {
public:
    explicit AuthDAO(sqlite3* db);

    bool createUser(const std::string &username, const std::string &password);
    bool checkCredentials(const std::string &username, const std::string &password);

private:
    std::string hashPassword(const std::string &password, const std::string &salt);
    std::string generateSalt();

    sqlite3* m_db;
};

} // namespace wizz
