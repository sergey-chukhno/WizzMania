#pragma once

#include "DatabaseTypes.h"
#include <sqlite3.h>
#include <string>
#include <vector>

namespace wizz {

class SocialDAO {
public:
    explicit SocialDAO(sqlite3* db);

    // Friend Management
    int addFriend(const std::string &username, const std::string &friendName);
    bool removeFriend(const std::string &username, const std::string &friendName);
    std::vector<std::string> getFriends(const std::string &username);
    std::vector<std::string> getFollowers(const std::string &username);

    // Messaging
    bool storeMessage(const std::string &sender, const std::string &recipient,
                      const std::string &body, bool isDelivered);
    std::vector<StoredMessage> fetchPendingMessages(const std::string &recipient);
    void markAsDelivered(int msgId);

    // Avatars
    bool updateUserAvatar(const std::string &username, const std::string &avatarPath);
    std::string getUserAvatar(const std::string &username);

    // Status
    bool updateCustomStatus(const std::string &username, const std::string &status);
    std::string getCustomStatus(const std::string &username);

private:
    sqlite3* m_db;
};

} // namespace wizz
