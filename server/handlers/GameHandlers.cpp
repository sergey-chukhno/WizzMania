#include "GameHandlers.h"
#include "../TcpServer.h"
#include "../ClientSession.h"
#include "../../common/Packet.h"
#include <iostream>

namespace wizz {

void GameStatusHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string gameName;
    uint32_t score;
    try {
        gameName = packet.readString();
        score = packet.readInt();
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    std::string username = session->getUsername();
    if (gameName.empty()) {
        server->getGameRoomManager().clearGameStatus(username);
    } else {
        server->getGameRoomManager().updateGameStatus(username, gameName, score);
    }

    // Use the cached contact list from the session — no DB round-trip needed.
    // This runs on the server's IO thread which is the same thread that processes
    // all session callbacks, so the contact cache is safe to read directly.
    Packet pkt(PacketType::GameStatus);
    pkt.writeString(username);
    pkt.writeString(gameName);
    pkt.writeInt(score);

    for (const auto& contactName : session->getContacts()) {
        ClientSession* targetSession = server->getSessionManager().getSessionByUsername(contactName);
        if (targetSession) {
            targetSession->sendPacket(pkt);
        }
    }
}


void GameInviteHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string target, gameName;
    try {
        target = packet.readString();
        gameName = packet.readString();
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    std::string senderName = session->getUsername();
    ClientSession *targetSession = server->getSessionManager().getSessionByUsername(target);
    if (targetSession) {
        Packet pkt(PacketType::GameInvite);
        pkt.writeString(senderName);
        pkt.writeString(gameName);
        targetSession->sendPacket(pkt);
        std::cout << "[Server] Routed GameInvite from " << senderName << " to " << target << std::endl;
    }
}

void GameInviteResponseHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string originalSender, gameName;
    bool accepted;
    try {
        originalSender = packet.readString();
        gameName = packet.readString();
        accepted = (packet.readInt() != 0);
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    std::string acceptorName = session->getUsername();
    ClientSession *originalSenderSession = server->getSessionManager().getSessionByUsername(originalSender);
    if (originalSenderSession) {
        Packet respPkt(PacketType::GameInviteResponse);
        respPkt.writeString(acceptorName);
        respPkt.writeString(gameName);
        respPkt.writeInt(accepted ? 1 : 0);
        originalSenderSession->sendPacket(respPkt);
    }

    if (accepted && originalSenderSession) {
        std::string roomId = std::to_string(std::time(nullptr)) + "_" + originalSender + "_" + acceptorName;
        server->getGameRoomManager().createRoom(roomId, originalSenderSession, session);

        Packet startUser1(PacketType::GameStart);
        startUser1.writeString(gameName);
        startUser1.writeString(roomId);
        startUser1.writeInt('X');
        startUser1.writeString(acceptorName);
        originalSenderSession->sendPacket(startUser1);

        Packet startUser2(PacketType::GameStart);
        startUser2.writeString(gameName);
        startUser2.writeString(roomId);
        startUser2.writeInt('O');
        startUser2.writeString(originalSender);
        session->sendPacket(startUser2);
    }
}

void GameMoveHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string roomId;
    uint8_t cellIndex;
    try {
        roomId = packet.readString();
        cellIndex = static_cast<uint8_t>(packet.readInt());
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    ClientSession *target = server->getGameRoomManager().getOpponent(roomId, session);
    if (target) {
        Packet pkt(PacketType::GameMove);
        pkt.writeString(roomId);
        pkt.writeInt(cellIndex);
        target->sendPacket(pkt);
    }
}

}
