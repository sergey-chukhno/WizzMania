#include "CryptoHandlers.h"
#include "../ClientSession.h"
#include "../TcpServer.h"
#include "../DatabaseManager.h"
#include "../../common/Packet.h"
#include <iostream>

namespace wizz {

void UploadPreKeysHandler::handle(ClientSession* session, Packet& pkt) {
    if (!session || !session->getServer() || session->getUsername().empty()) return;

    // Decode public keys
    std::string identityKey = pkt.readString();
    std::string signedPreKey = pkt.readString();
    std::string signature = pkt.readString();
    
    // Decode array of One-Time Pre-Keys
    uint32_t otkCount = pkt.readInt();
    std::vector<std::pair<int, std::string>> otks;
    for (uint32_t i = 0; i < otkCount; ++i) {
        int id = pkt.readInt();
        std::string pub = pkt.readString();
        otks.push_back({id, pub});
    }

    auto& db = session->getServer()->getDb();
    
    std::string username = session->getUsername();
    // Post DB access to background worker thread
    db.postTask([&db, username, identityKey, signedPreKey, signature, otks, session]() {
        bool ok1 = db.storeUserKeys(username, identityKey, signedPreKey, signature);
        bool ok2 = db.storeOneTimeKeys(username, otks);
            
            if (ok1 && ok2) {
                // Post network response back to networking thread safely
                session->getServer()->postResponse([session]() {
                    Packet ack(PacketType::PreKeysUploaded);
                    session->sendPacket(ack);
                });
                std::cout << "[Crypto] Keys uploaded for " << username << " (" << otks.size() << " OTKs)" << std::endl;
            }
        });
    }

void FetchPreKeyBundleHandler::handle(ClientSession* session, Packet& pkt) {
    if (!session || !session->getServer() || session->getUsername().empty()) return;

    std::string targetUsername = pkt.readString();
    
    auto& db = session->getServer()->getDb();
    
    // DB worker thread lookup
    db.postTask([&db, targetUsername, session]() {
        auto bundle = db.fetchPreKeyBundle(targetUsername);
            
            // Post result back to async networking thread
            session->getServer()->postResponse([targetUsername, bundle, session]() {
                Packet res(PacketType::PreKeyBundleResponse);
                res.writeString(targetUsername);
                res.writeString(bundle.identityKey);
                res.writeString(bundle.signedPreKey);
                res.writeString(bundle.signedPreKeySignature);
                res.writeInt(bundle.oneTimeKeyId);
                res.writeString(bundle.oneTimeKey); // Might be empty if none left
                
                session->sendPacket(res);
            });
        std::cout << "[Crypto] Dispatched Key Bundle of " << targetUsername << " to " << session->getUsername() << std::endl;
    });
}

} // namespace wizz
