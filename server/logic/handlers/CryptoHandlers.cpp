#include "CryptoHandlers.h"
#include "../../core/ClientSession.h"
#include "../../core/TcpServer.h"
#include "../../data/DatabaseManager.h"
#include "../../../common/Packet.h"
#include <iostream>

#include <algorithm>
#include <cctype>

namespace wizz {

static std::string normalize(const std::string& str) {
    std::string data = str;
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return data;
}

void UploadPreKeysHandler::handle(ClientSession* session, Packet& pkt) {
    if (!session || !session->getServer() || session->getUsername().empty()) return;

    // Decode public keys (Binary Safe 33/64 byte reads)
    std::string identityKey = std::string(reinterpret_cast<const char*>(pkt.readBytes(33).data()), 33);
    std::string signedPreKey = std::string(reinterpret_cast<const char*>(pkt.readBytes(33).data()), 33);
    std::string signature = std::string(reinterpret_cast<const char*>(pkt.readBytes(64).data()), 64);
    int registrationId = pkt.readInt();
    
    // Decode array of One-Time Pre-Keys
    uint32_t otkCount = pkt.readInt();
    std::vector<std::pair<int, std::string>> otks;
    for (uint32_t i = 0; i < otkCount; ++i) {
        int id = pkt.readInt();
        std::string pub = std::string(reinterpret_cast<const char*>(pkt.readBytes(33).data()), 33);
        otks.push_back({id, pub});
    }

    auto* server = session->getServer();
    std::string username = normalize(session->getUsername());
    
    // Post DB access to background worker thread
    server->getDb().postTask([server, username, identityKey, signedPreKey, signature, registrationId, otks, session]() {
        server->getDb().crypto()->clearUserKeys(username);
        bool ok1 = server->getDb().crypto()->storeUserKeys(username, identityKey, signedPreKey, signature, registrationId);
        bool ok2 = server->getDb().crypto()->storeOneTimeKeys(username, otks);
            
        if (ok1 && ok2) {
            // Post network response back to networking thread safely
            server->postResponse([session]() {
                Packet ack(PacketType::PreKeysUploaded);
                session->sendPacket(ack);
            });
            std::cout << "[Crypto] Keys uploaded for " << username << " (" << otks.size() << " OTKs)" << std::endl;
        }
    });
}

void FetchPreKeyBundleHandler::handle(ClientSession* session, Packet& pkt) {
    if (!session || !session->getServer() || session->getUsername().empty()) return;

    std::string targetUsername = normalize(pkt.readString());
    auto* server = session->getServer();
    
    // DB worker thread lookup
    server->getDb().postTask([server, targetUsername, session]() {
        auto bundle = server->getDb().crypto()->fetchPreKeyBundle(targetUsername);
            
        // Post result back to async networking thread
        server->postResponse([targetUsername, bundle, session]() {
            if (bundle.identityKey.empty()) {
                std::cerr << "[Crypto] WARNING: Requested bundle for " << targetUsername << " is EMPTY. User has not uploaded keys!" << std::endl;
            }
            
            Packet res(PacketType::PreKeyBundleResponse);
            res.writeString(targetUsername);
            
            // Binary write for raw cryptographic material
            res.writeData(reinterpret_cast<const uint8_t*>(bundle.identityKey.data()), bundle.identityKey.size());
            res.writeData(reinterpret_cast<const uint8_t*>(bundle.signedPreKey.data()), bundle.signedPreKey.size());
            res.writeData(reinterpret_cast<const uint8_t*>(bundle.signedPreKeySignature.data()), bundle.signedPreKeySignature.size());
            
            res.writeInt(bundle.registrationId);
            res.writeInt(bundle.oneTimeKeyId);
            
            // OTK might be empty if exhausted
            if (!bundle.oneTimeKey.empty()) {
                res.writeData(reinterpret_cast<const uint8_t*>(bundle.oneTimeKey.data()), bundle.oneTimeKey.size());
            } else {
                res.writeData(nullptr, 0);
            }
            
            session->sendPacket(res);
        });
        std::cout << "[Crypto] Dispatched Key Bundle of " << targetUsername << " to " << session->getUsername() << std::endl;
    });
}

} // namespace wizz
