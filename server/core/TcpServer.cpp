#include "TcpServer.h"
#include "../logic/handlers/AuthHandlers.h"
#include "../logic/handlers/SocialHandlers.h"
#include "../logic/handlers/GameHandlers.h"
#include "../logic/handlers/CryptoHandlers.h"
#include "../observability/MetricsManager.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace wizz {

namespace fs = std::filesystem;

TcpServer::TcpServer(int port) : TcpServer(port, "wizzmania.db") {}

TcpServer::TcpServer(int port, const std::string& dbPath)
    : m_ioContext(),
      m_sslContext(asio::ssl::context::tlsv12),
      m_acceptor(m_ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      m_port(port),
      m_isRunning(false), m_db(dbPath) {
  m_sslContext.set_options(asio::ssl::context::default_workarounds |
                           asio::ssl::context::no_sslv2 |
                           asio::ssl::context::single_dh_use);
  // Flexible Path Detection for Certificates
  std::string certPath = "server/certs/server.crt";
  std::string keyPath = "server/certs/server.key";

  if (!fs::exists(certPath)) {
      // Try relative to build folder
      certPath = "../server/certs/server.crt";
      keyPath = "../server/certs/server.key";
  }

  if (!fs::exists(certPath)) {
      // Try relative to deep test folder (build/tests/server/)
      certPath = "../../server/certs/server.crt";
      keyPath = "../../server/certs/server.key";
  }

  if (!fs::exists(certPath)) {
      // Try relative to even deeper test folder (build/tests/server/ if run from build root)
      certPath = "../../../server/certs/server.crt";
      keyPath = "../../../server/certs/server.key";
  }

  if (!fs::exists(certPath)) {
      throw std::runtime_error("SSL Certificates not found! Expected at 'server/certs/', '../server/certs/', '../../server/certs/', or '../../../server/certs/'.");
  }

  m_sslContext.use_certificate_chain_file(certPath);
  m_sslContext.use_private_key_file(keyPath, asio::ssl::context::pem);

  m_packetRouter.registerHandler(PacketType::Login, std::make_unique<LoginHandler>());
  m_packetRouter.registerHandler(PacketType::Register, std::make_unique<RegisterHandler>());
  m_packetRouter.registerHandler(PacketType::E2EMessage, std::make_unique<E2EMessageHandler>());
  m_packetRouter.registerHandler(PacketType::Nudge, std::make_unique<NudgeHandler>());
  m_packetRouter.registerHandler(PacketType::VoiceMessage, std::make_unique<VoiceMessageHandler>());
  m_packetRouter.registerHandler(PacketType::TypingIndicator, std::make_unique<TypingIndicatorHandler>());
  m_packetRouter.registerHandler(PacketType::ContactStatusChange, std::make_unique<StatusChangeHandler>());
  m_packetRouter.registerHandler(PacketType::UpdateStatus, std::make_unique<UpdateStatusHandler>());
  m_packetRouter.registerHandler(PacketType::UpdateAvatar, std::make_unique<UpdateAvatarHandler>());
  m_packetRouter.registerHandler(PacketType::GetAvatar, std::make_unique<GetAvatarHandler>());
  m_packetRouter.registerHandler(PacketType::AddContact, std::make_unique<AddContactHandler>());
  m_packetRouter.registerHandler(PacketType::RemoveContact, std::make_unique<RemoveContactHandler>());
  m_packetRouter.registerHandler(PacketType::GameStatus, std::make_unique<GameStatusHandler>());
  m_packetRouter.registerHandler(PacketType::GameInvite, std::make_unique<GameInviteHandler>());
  m_packetRouter.registerHandler(PacketType::GameInviteResponse, std::make_unique<GameInviteResponseHandler>());
  m_packetRouter.registerHandler(PacketType::GameMove, std::make_unique<GameMoveHandler>());

  // E2EE
  m_packetRouter.registerHandler(PacketType::UploadPreKeys, std::make_unique<UploadPreKeysHandler>());
  m_packetRouter.registerHandler(PacketType::FetchPreKeyBundle, std::make_unique<FetchPreKeyBundleHandler>());
}

TcpServer::~TcpServer() { stop(); }

void TcpServer::start() {
  try {
    if (!m_db.init()) {
      throw std::runtime_error("Failed to initialize Database!");
    }

    setupVoiceStorage();

    std::cout << "[Server] Listening on port " << m_port << std::endl;
    m_isRunning = true;

    doAccept();

    // SENTINEL PHASE: Move networking to a background thread
    m_networkThread = std::thread([this]() {
        run();
    });
  } catch (const std::exception &e) {
    std::cerr << "[Server] Fatal Error: " << e.what() << std::endl;
    stop();
    throw;
  }
}

void TcpServer::stop() {
  m_isRunning = false;
  m_ioContext.stop();
  if (m_networkThread.joinable()) {
    m_networkThread.join();
  }
  std::cout << "[Server] Stopped." << std::endl;
}

ClientSession *TcpServer::getSession(int sessionId) {
  return m_sessionManager.getSessionById(sessionId);
}

void TcpServer::doAccept() {
  m_acceptor.async_accept([this](asio::error_code ec,
                                 asio::ip::tcp::socket socket) {
    if (!ec) {
      std::string remoteIp = socket.remote_endpoint().address().to_string();
      
      // SENTINEL PHASE: Track connection attempt
      MetricsManager::getInstance().increment(MetricType::ConnectionsTotal);

      // SHIELD PHASE: Connection Rate Limiting
      {
          std::lock_guard<std::mutex> lock(m_limiterMutex);
          if (m_ipLimiters.find(remoteIp) == m_ipLimiters.end()) {
              m_ipLimiters[remoteIp] = std::make_shared<RateLimiter>(CONN_BURST, CONN_RATE);
          }
          
          if (!m_ipLimiters[remoteIp]->consume(1.0)) {
              MetricsManager::getInstance().increment(MetricType::ShieldDrops);
              std::cerr << "[Shield] Dropping connection from " << remoteIp << " (Rate limit exceeded)" << std::endl;
              socket.close();
              doAccept();
              return;
          }
      }

      int sessionId = m_nextSessionId++;

      try {
        // Use explicit new to ensure shared_ptr establishes the control block clearly
        auto session = std::shared_ptr<ClientSession>(new ClientSession(
            sessionId, std::move(socket), m_sslContext, this));

        m_sessionManager.addSession(sessionId, session);
        MetricsManager::getInstance().increment(MetricType::ActiveSessions);
        session->start();
        
        doAccept();
      } catch (const std::exception& e) {
        std::cerr << "[Server] Error creating session: " << e.what() << std::endl;
      }
    } else {
      std::cerr << "[Server] Accept Error: " << ec.message() << std::endl;
    }
  });
}

void TcpServer::run() {
  m_ioContext.run();
}

void TcpServer::cleanup() {
}

void TcpServer::setupVoiceStorage() {
  std::string dir = "server/storage/avatars";
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }
}

void TcpServer::handleDisconnect(int sessionId) {
  ClientSession* session = m_sessionManager.getSessionById(sessionId);
  if (!session) return;

  std::string username = session->getUsername();
  m_sessionManager.removeSession(sessionId);
  MetricsManager::getInstance().decrement(MetricType::ActiveSessions);
  if (!username.empty()) {
    m_sessionManager.setUserOffline(username);
    m_sessionManager.updateStatus(username, wizz::UserStatus::Offline);
    std::cout << "[Server] User Offline: " << username << std::endl;
    
    m_db.postTask([this, username]() {
      auto followers = m_db.social()->getFollowers(username);
      auto friends = m_db.social()->getFriends(username);

      postResponse([
          this, username, followers = std::move(followers),
          friends = std::move(friends)]() {

        Packet notify(PacketType::ContactStatusChange);
        notify.writeInt(static_cast<uint32_t>(wizz::UserStatus::Offline));
        notify.writeString(username);
        notify.writeString("");

        std::set<std::string> contacts;
        for (const auto &f : followers) contacts.insert(f);
        for (const auto &f : friends)   contacts.insert(f);

        for (const auto &contactName : contacts) {
          ClientSession *target = m_sessionManager.getSessionByUsername(contactName);
          if (target) {
            target->sendPacket(notify);
          }
        }
      });
    });
  }
}

void TcpServer::broadcastMessage(const std::string& sender, const std::string& message) {
  // Post this to the IO thread to ensure thread-safety when touching sockets
  postResponse([this, sender, message]() {
    Packet pkt(PacketType::DirectMessage); // Reuse DirectMessage packet for simplicity
    pkt.writeString(sender);
    pkt.writeString(message);
    
    auto sessions = m_sessionManager.getAllOnlineSessions();
    for (auto* session : sessions) {
      if (session) {
        session->sendPacket(pkt);
      }
    }
  });
}

} // namespace wizz
