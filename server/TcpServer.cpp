#include "TcpServer.h"
#include "handlers/AuthHandlers.h"
#include "handlers/SocialHandlers.h"
#include "handlers/GameHandlers.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace wizz {

namespace fs = std::filesystem;

TcpServer::TcpServer(int port)
    : m_ioContext(),
      m_sslContext(asio::ssl::context::tlsv12),
      m_acceptor(m_ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      m_port(port),
      m_isRunning(false), m_db("wizzmania.db") {
  m_sslContext.set_options(asio::ssl::context::default_workarounds |
                           asio::ssl::context::no_sslv2 |
                           asio::ssl::context::single_dh_use);
  m_sslContext.use_certificate_chain_file("server/certs/server.crt");
  m_sslContext.use_private_key_file("server/certs/server.key",
                                    asio::ssl::context::pem);

  m_packetRouter.registerHandler(PacketType::Login, std::make_unique<LoginHandler>());
  m_packetRouter.registerHandler(PacketType::Register, std::make_unique<RegisterHandler>());
  m_packetRouter.registerHandler(PacketType::DirectMessage, std::make_unique<MessageHandler>());
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

    run();
  } catch (const std::exception &e) {
    std::cerr << "[Server] Fatal Error: " << e.what() << std::endl;
    stop();
    throw;
  }
}

void TcpServer::stop() {
  m_isRunning = false;
  m_ioContext.stop();
  std::cout << "[Server] Stopped." << std::endl;
}

ClientSession *TcpServer::getSession(int sessionId) {
  return m_sessionManager.getSessionById(sessionId);
}

void TcpServer::doAccept() {
  m_acceptor.async_accept([this](asio::error_code ec,
                                 asio::ip::tcp::socket socket) {
    if (!ec) {
      int sessionId = m_nextSessionId++;
      std::cout << "[Server] New Connection (Session ID: " << sessionId << ")"
                << std::endl;

      auto session = std::make_shared<ClientSession>(
          sessionId, std::move(socket), m_sslContext, this);

      m_sessionManager.addSession(sessionId, session);
      session->start();

      doAccept();
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
  if (!username.empty()) {
    m_sessionManager.setUserOffline(username);
    m_sessionManager.updateStatus(username, 3);
    std::cout << "[Server] User Offline: " << username << std::endl;
    
    m_db.postTask([this, username]() {
      auto followers = m_db.getFollowers(username);
      auto friends = m_db.getFriends(username);

      postResponse([
          this, username, followers = std::move(followers),
          friends = std::move(friends)]() {

        Packet notify(PacketType::ContactStatusChange);
        notify.writeInt(3);
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

} // namespace wizz
