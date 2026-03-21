#include "SocialHandlers.h"
#include "../TcpServer.h"
#include "../ClientSession.h"
#include "../../common/Packet.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace wizz {

void MessageHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser, messageBody;
    try {
        targetUser = packet.readString();
        messageBody = packet.readString();
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    bool delivered = false;
    ClientSession *targetSession = server->getSessionManager().getSessionByUsername(targetUser);
    if (targetSession) {
        Packet outPacket(PacketType::DirectMessage);
        outPacket.writeString(session->getUsername());
        outPacket.writeString(messageBody);
        targetSession->sendPacket(outPacket);
        delivered = true;
    }
    
    server->getDb().postTask([server, senderName = session->getUsername(), targetUser, messageBody, delivered]() {
        server->getDb().storeMessage(senderName, targetUser, messageBody, delivered);
    });
}

void NudgeHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser;
    try { targetUser = packet.readString(); } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    int status = server->getSessionManager().getStatus(targetUser);
    ClientSession *targetSession = server->getSessionManager().getSessionByUsername(targetUser);
    bool isOnline = (targetSession != nullptr);

    if (!isOnline) {
        Packet err(PacketType::Error);
        err.writeString("User " + targetUser + " is offline.");
        session->sendPacket(err);
        return;
    }
    if (status == 2) { // Busy
        Packet err(PacketType::Error);
        err.writeString("User " + targetUser + " is busy and cannot be nudged.");
        session->sendPacket(err);
        return;
    }
    Packet p(PacketType::Nudge);
    p.writeString(session->getUsername());
    targetSession->sendPacket(p);
}

void VoiceMessageHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser;
    uint32_t duration;
    std::vector<uint8_t> data;
    try {
        targetUser = packet.readString();
        duration = packet.readInt();
        uint32_t size = packet.readInt();
        data = packet.readBytes(size);
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    long long timestamp = std::time(nullptr);
    std::filesystem::path storageDir = std::filesystem::path("server") / "storage";
    std::string filename = "voice_" + session->getUsername() + "_" + std::to_string(timestamp) + ".wav";
    std::string filepath = (storageDir / filename).string();

    std::ofstream outfile(filepath, std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
        outfile.close();
    }

    ClientSession *targetSession = server->getSessionManager().getSessionByUsername(targetUser);
    if (targetSession) {
        Packet p(PacketType::VoiceMessage);
        p.writeString(session->getUsername());
        p.writeInt(static_cast<uint32_t>(duration));
        p.writeInt(static_cast<uint32_t>(data.size()));
        p.writeData(data.data(), data.size());
        targetSession->sendPacket(p);
    } else {
        std::string proxyMsg = "VOICE:" + std::to_string(duration) + ":" + filepath;
        server->getDb().postTask([server, senderName = session->getUsername(), targetUser, proxyMsg]() {
            server->getDb().storeMessage(senderName, targetUser, proxyMsg, false);
        });
    }
}

void TypingIndicatorHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser;
    bool isTyping;
    try {
        targetUser = packet.readString();
        isTyping = (packet.readInt() != 0);
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    ClientSession *targetSession = server->getSessionManager().getSessionByUsername(targetUser);
    if (targetSession) {
        Packet p(PacketType::TypingIndicator);
        p.writeString(session->getUsername());
        p.writeInt(isTyping ? 1 : 0);
        targetSession->sendPacket(p);
    }
}

void StatusChangeHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    int newStatus;
    try {
        newStatus = static_cast<int>(packet.readInt());
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    std::string username = session->getUsername();
    server->getSessionManager().updateStatus(username, newStatus);

    server->getDb().postTask([server, username, newStatus]() {
        auto followers = server->getDb().getFollowers(username);
        auto friends = server->getDb().getFriends(username);

        server->postResponse([server, username, newStatus, followers = std::move(followers), friends = std::move(friends)]() {
            std::string customStatus = server->getSessionManager().getCustomStatus(username);
            std::set<std::string> contacts;
            for (const auto &f : followers) contacts.insert(f);
            for (const auto &f : friends) contacts.insert(f);

            for (const auto &contactName : contacts) {
                ClientSession *targetSession = server->getSessionManager().getSessionByUsername(contactName);
                if (targetSession) {
                    Packet notify(PacketType::ContactStatusChange);
                    notify.writeInt(static_cast<uint32_t>(newStatus));
                    notify.writeString(username);
                    notify.writeString(customStatus);
                    targetSession->sendPacket(notify);
                }
            }
        });
    });
}

void UpdateStatusHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string statusMsg;
    try { statusMsg = packet.readString(); } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    std::string username = session->getUsername();
    server->getSessionManager().updateCustomStatus(username, statusMsg);

    server->getDb().postTask([server, username, statusMsg]() {
        server->getDb().updateCustomStatus(username, statusMsg);
        auto followers = server->getDb().getFollowers(username);
        auto friends = server->getDb().getFriends(username);

        server->postResponse([server, username, statusMsg, followers = std::move(followers), friends = std::move(friends)]() {
            std::set<std::string> contacts;
            for (const auto &f : followers) contacts.insert(f);
            for (const auto &f : friends) contacts.insert(f);

            int currentStatus = server->getSessionManager().getStatus(username);
            Packet notify(PacketType::ContactStatusChange);
            notify.writeInt(static_cast<uint32_t>(currentStatus));
            notify.writeString(username);
            notify.writeString(statusMsg);

            for (const auto &contactName : contacts) {
                ClientSession *targetSession = server->getSessionManager().getSessionByUsername(contactName);
                if (targetSession) targetSession->sendPacket(notify);
            }
        });
    });
}

void UpdateAvatarHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::vector<uint8_t> data;
    try {
        uint32_t size = packet.readInt();
        data = packet.readBytes(size);
    } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;

    std::string username = session->getUsername();
    long long timestamp = std::time(nullptr);
    std::filesystem::path storageDir = std::filesystem::path("server") / "storage" / "avatars";

    if (!std::filesystem::exists(storageDir)) std::filesystem::create_directories(storageDir);

    std::string filename = "avatar_" + username + "_" + std::to_string(timestamp) + ".png";
    std::string filepath = (storageDir / filename).string();

    std::ofstream outfile(filepath, std::ios::binary);
    if (!outfile.is_open()) return;
    outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
    outfile.close();

    server->getDb().postTask([server, username, filepath, data]() {
        if (server->getDb().updateUserAvatar(username, filepath)) {
            auto friends = server->getDb().getFriends(username);
            server->postResponse([server, username, friends = std::move(friends), data]() {
                for (const auto &friendName : friends) {
                    ClientSession *targetSession = server->getSessionManager().getSessionByUsername(friendName);
                    if (targetSession) {
                        Packet resp(PacketType::AvatarData);
                        resp.writeString(username);
                        resp.writeInt(static_cast<uint32_t>(data.size()));
                        resp.writeData(data.data(), data.size());
                        targetSession->sendPacket(resp);
                    }
                }
            });
        }
    });
}

void GetAvatarHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser;
    try { targetUser = packet.readString(); } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;
    int sessionId = session->getId();

    server->getDb().postTask([server, sessionId, targetUser]() {
        std::string filepath = server->getDb().getUserAvatar(targetUser);
        std::vector<uint8_t> buffer;
        if (!filepath.empty() && std::filesystem::exists(std::filesystem::path(filepath))) {
            std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
            if (infile.is_open()) {
                std::streamsize size = infile.tellg();
                infile.seekg(0, std::ios::beg);
                buffer.resize(size);
                infile.read(reinterpret_cast<char *>(buffer.data()), size);
            }
        }

        server->postResponse([server, sessionId, targetUser, filepath, buffer = std::move(buffer)]() {
            ClientSession *s = server->getSession(sessionId);
            if (!s || filepath.empty() || buffer.empty()) return;

            Packet resp(PacketType::AvatarData);
            resp.writeString(targetUser);
            resp.writeInt(static_cast<uint32_t>(buffer.size()));
            resp.writeData(buffer.data(), buffer.size());
            s->sendPacket(resp);
        });
    });
}

void AddContactHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser;
    try { targetUser = packet.readString(); } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;
    int sessionId = session->getId();
    std::string username = session->getUsername();

    server->getDb().postTask([server, sessionId, username, targetUser]() {
        bool ok = server->getDb().addFriend(username, targetUser);
        std::vector<std::string> friends;
        if (ok) friends = server->getDb().getFriends(username);

        server->postResponse([server, sessionId, ok, friends = std::move(friends)]() {
            ClientSession *s = server->getSession(sessionId);
            if (!s) return;

            if (ok) {
                Packet resp(PacketType::ContactList);
                resp.writeInt(static_cast<uint32_t>(friends.size()));
                for (const auto &name : friends) {
                    resp.writeString(name);
                    resp.writeInt(static_cast<uint32_t>(server->getSessionManager().getStatus(name)));
                    resp.writeString(server->getSessionManager().getCustomStatus(name));
                }
                s->sendPacket(resp);
            } else {
                Packet err(PacketType::Error);
                err.writeString("Failed to add contact: User not found.");
                s->sendPacket(err);
            }
        });
    });
}

void RemoveContactHandler::handle(ClientSession* session, Packet& packet) {
    if (!session->isLoggedIn()) return;
    std::string targetUser;
    try { targetUser = packet.readString(); } catch (...) { return; }

    TcpServer* server = session->getServer();
    if (!server) return;
    int sessionId = session->getId();
    std::string username = session->getUsername();

    server->getDb().postTask([server, sessionId, username, targetUser]() {
        bool ok = server->getDb().removeFriend(username, targetUser);
        std::vector<std::string> friends;
        if (ok) friends = server->getDb().getFriends(username);

        server->postResponse([server, sessionId, ok, friends = std::move(friends)]() {
            ClientSession *s = server->getSession(sessionId);
            if (!s) return;

            if (ok) {
                Packet resp(PacketType::ContactList);
                resp.writeInt(static_cast<uint32_t>(friends.size()));
                for (const auto &name : friends) {
                    resp.writeString(name);
                    resp.writeInt(static_cast<uint32_t>(server->getSessionManager().getStatus(name)));
                    resp.writeString(server->getSessionManager().getCustomStatus(name));
                }
                s->sendPacket(resp);
            }
        });
    });
}

}
