#pragma once

#include "IPacketHandler.h"

namespace wizz {

class UploadPreKeysHandler : public IPacketHandler {
public:
    void handle(ClientSession* session, Packet& packet) override;
};

class FetchPreKeyBundleHandler : public IPacketHandler {
public:
    void handle(ClientSession* session, Packet& packet) override;
};

} // namespace wizz
