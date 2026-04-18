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

} // namespace wizz
