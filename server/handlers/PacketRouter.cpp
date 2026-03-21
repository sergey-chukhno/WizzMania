#include "PacketRouter.h"
#include "../ClientSession.h"
#include <iostream>

namespace wizz {

PacketRouter::PacketRouter() {}

void PacketRouter::registerHandler(PacketType type, std::unique_ptr<IPacketHandler> handler) {
    m_handlers[type] = std::move(handler);
}

void PacketRouter::handle(ClientSession* session, Packet& packet) {
    auto it = m_handlers.find(packet.type());
    if (it != m_handlers.end()) {
        it->second->handle(session, packet);
    } else {
        std::cout << "[Router] No handler registered for packet type: " 
                  << static_cast<int>(packet.type()) << std::endl;
    }
}

}
