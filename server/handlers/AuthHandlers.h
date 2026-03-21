#pragma once

#include "IPacketHandler.h"

namespace wizz {

class LoginHandler : public IPacketHandler {
public:
    void handle(ClientSession* session, Packet& packet) override;
};

class RegisterHandler : public IPacketHandler {
public:
    void handle(ClientSession* session, Packet& packet) override;
};

}
