#pragma once

namespace wizz {

class ClientSession;
class Packet;

class IPacketHandler {
public:
    virtual ~IPacketHandler() = default;
    virtual void handle(ClientSession* session, Packet& packet) = 0;
};

}
