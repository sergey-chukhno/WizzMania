#pragma once

#include <string>

namespace wizz {

struct StoredMessage {
    int id;
    std::string sender;
    std::string body;
    std::string timestamp;
};

struct PreKeyBundle {
    std::string identityKey;
    std::string signedPreKey;
    std::string signedPreKeySignature;
    int registrationId;
    int oneTimeKeyId; // -1 if exhausted
    std::string oneTimeKey; // empty if exhausted
};

struct RichPresence {
    int type = 0; // 0=None, 1=Gaming, 2=Music, 3=Thinking
    std::string activityName;
    std::string activityDetail;
    int64_t startTime = 0;
};

} // namespace wizz
