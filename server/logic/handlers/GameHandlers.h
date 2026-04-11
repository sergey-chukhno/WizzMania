#pragma once
#include "IPacketHandler.h"

namespace wizz {

class GameStatusHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class GameInviteHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class GameInviteResponseHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class GameMoveHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };

}
