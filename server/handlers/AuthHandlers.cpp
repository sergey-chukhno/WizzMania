#include "AuthHandlers.h"
#include "../TcpServer.h"
#include "../ClientSession.h"
#include "../../common/Packet.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

namespace wizz {

void LoginHandler::handle(ClientSession* session, Packet& packet) {
    std::string username, password, customStatus;
    try {
        username = packet.readString();
        password = packet.readString();
    } catch (const std::exception &e) {
        std::cerr << "[LoginHandler] Protocol Error: " << e.what() << std::endl;
        return;
    }

    TcpServer* server = session->getServer();
    if (!server) return;
    int sessionId = session->getId();

    server->getDb().postTask([server, username, password, sessionId]() {
        bool ok = server->getDb().checkCredentials(username, password);
        if (!ok) {
            server->postResponse([server, sessionId]() {
                ClientSession *s = server->getSession(sessionId);
                if (s) {
                    Packet failPkt(PacketType::LoginFailed);
                    failPkt.writeString("Invalid Username or Password");
                    s->sendPacket(failPkt);
                }
            });
            return;
        }

        auto pending = server->getDb().fetchPendingMessages(username);
        for (const auto &msg : pending) {
            server->getDb().markAsDelivered(msg.id);
        }
        auto followers = server->getDb().getFollowers(username);
        auto friends = server->getDb().getFriends(username);
        auto dbCustomStatus = server->getDb().getCustomStatus(username);

        server->postResponse([server, username, sessionId, 
                              customStatus = std::move(dbCustomStatus),
                              pending = std::move(pending),
                              followers = std::move(followers),
                              friends = std::move(friends)]() {
            ClientSession *s = server->getSession(sessionId);
            if (!s) return;

            s->setLoggedIn(true);
            s->setUsername(username);
            std::cout << "[Server] User Online: " << username << std::endl;
            server->getSessionManager().setUserOnline(username, s, customStatus);

            // Cache the full contact list in the session for fast broadcasts (avoids repeated DB queries)
            std::set<std::string> contactSet;
            for (const auto &f : followers) contactSet.insert(f);
            for (const auto &f : friends) contactSet.insert(f);
            s->setContacts(std::move(contactSet));

            Packet resp(PacketType::LoginSuccess);
            s->sendPacket(resp);

            std::set<std::string> contacts;
            for (const auto &f : followers) contacts.insert(f);
            for (const auto &f : friends) contacts.insert(f);

            for (const auto &contactName : contacts) {
                ClientSession* targetSession = server->getSessionManager().getSessionByUsername(contactName);
                if (targetSession) {
                    std::cout << "[Server] Broadcasting Online Status of " << username
                              << " to contact " << contactName << std::endl;
                    Packet notify(PacketType::ContactStatusChange);
                    notify.writeInt(0); // Online
                    notify.writeString(username);
                    notify.writeString(customStatus);
                    targetSession->sendPacket(notify);
                }
            }

            if (!friends.empty()) {
                Packet contactList(PacketType::ContactList);
                contactList.writeInt(static_cast<uint32_t>(friends.size()));
                for (const auto &friendName : friends) {
                    contactList.writeString(friendName);
                    contactList.writeInt(static_cast<uint32_t>(server->getSessionManager().getStatus(friendName)));
                    contactList.writeString(server->getSessionManager().getCustomStatus(friendName));
                }
                s->sendPacket(contactList);
            }

            for (const auto& onlineUser : server->getSessionManager().getAllOnlineUsernames()) {
                if (onlineUser == username) continue;
                bool isInterested = (contacts.find(onlineUser) != contacts.end());
                
                if (isInterested) {
                    int status = server->getSessionManager().getStatus(onlineUser);
                    Packet notify(PacketType::ContactStatusChange);
                    notify.writeInt(status);
                    notify.writeString(onlineUser);
                    notify.writeString(server->getSessionManager().getCustomStatus(onlineUser));
                    s->sendPacket(notify);

                    std::string gameName;
                    uint32_t score = 0;
                    if (server->getGameRoomManager().getGameStatus(onlineUser, gameName, score)) {
                        Packet gamePkt(PacketType::GameStatus);
                        gamePkt.writeString(onlineUser);
                        gamePkt.writeString(gameName);
                        gamePkt.writeInt(score);
                        s->sendPacket(gamePkt);
                    }
                }
            }

            if (!pending.empty()) {
                std::cout << "[Server] Flushing " << pending.size()
                          << " offline messages to " << username << std::endl;
                for (const auto &msg : pending) {
                    if (msg.body.rfind("VOICE:", 0) == 0) {
                        std::vector<std::string> parts;
                        std::stringstream ss(msg.body);
                        std::string item;
                        while (std::getline(ss, item, ':')) {
                            parts.push_back(item);
                        }
                        if (parts.size() >= 3) {
                            uint16_t duration = static_cast<uint16_t>(std::stoi(parts[1]));
                            std::string filename = parts[2];
                            std::ifstream infile(filename, std::ios::binary | std::ios::ate);
                            if (infile.is_open()) {
                                std::streamsize size = infile.tellg();
                                infile.seekg(0, std::ios::beg);
                                std::vector<uint8_t> buffer(size);
                                if (infile.read(reinterpret_cast<char *>(buffer.data()), size)) {
                                    Packet outPacket(PacketType::VoiceMessage);
                                    outPacket.writeString(msg.sender);
                                    outPacket.writeInt(static_cast<uint32_t>(duration));
                                    outPacket.writeInt(static_cast<uint32_t>(buffer.size()));
                                    outPacket.writeData(buffer.data(), buffer.size());
                                    s->sendPacket(outPacket);
                                }
                            }
                        }
                    } else {
                        Packet outPacket(PacketType::DirectMessage);
                        outPacket.writeString(msg.sender);
                        outPacket.writeString(msg.body);
                        s->sendPacket(outPacket);
                    }
                }
            }
        });
    });
}

void RegisterHandler::handle(ClientSession* session, Packet& packet) {
    std::string username, password;
    try {
        username = packet.readString();
        password = packet.readString();
    } catch (const std::exception &e) {
        std::cerr << "[Session] Register Protocol Error: " << e.what() << std::endl;
        return;
    }

    std::cout << "[Session] Register Attempt: " << username << std::endl;

    TcpServer* server = session->getServer();
    if (!server) return;
    int sessionId = session->getId();

    server->getDb().postTask([server, sessionId, username, password]() {
        bool ok = server->getDb().createUser(username, password);

        server->postResponse([server, sessionId, username, ok]() {
            ClientSession *s = server->getSession(sessionId);
            if (!s) return;

            if (ok) {
                std::cout << "[Server] Registered: " << username << std::endl;
                Packet resp(PacketType::RegisterSuccess);
                resp.writeString("Registration Successful!");
                s->sendPacket(resp);
            } else {
                std::cout << "[Server] Registration Failed: " << username << std::endl;
                Packet resp(PacketType::RegisterFailed);
                resp.writeString("Username already taken.");
                s->sendPacket(resp);
            }
        });
    });
}

}
