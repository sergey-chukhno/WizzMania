#pragma once
#include "IPacketHandler.h"

namespace wizz {

class MessageHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class NudgeHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class VoiceMessageHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class TypingIndicatorHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class StatusChangeHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class UpdateStatusHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class UpdateAvatarHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class GetAvatarHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class AddContactHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };
class RemoveContactHandler : public IPacketHandler { public: void handle(ClientSession* session, Packet& packet) override; };

}
