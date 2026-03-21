#pragma once

#include <unordered_map>
#include <memory>
#include "../../common/Types.h"
#include "../../common/Packet.h"
#include "IPacketHandler.h"

namespace wizz {

class PacketRouter {
public:
    PacketRouter();
    ~PacketRouter() = default;

    void registerHandler(PacketType type, std::unique_ptr<IPacketHandler> handler);
    void handle(ClientSession* session, Packet& packet);

private:
    std::unordered_map<PacketType, std::unique_ptr<IPacketHandler>> m_handlers;
};

}
