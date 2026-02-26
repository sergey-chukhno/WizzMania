#pragma once

#include "../common/Packet.h"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <deque>
#include <functional> // For std::function
#include <memory>
#include <string>
#include <vector>

// Forward Declaration
namespace wizz {
class TcpServer;
}

namespace wizz {

class ClientSession; // Forward decl
using OnLoginCallback = std::function<void(std::shared_ptr<ClientSession>)>;
// Callback for Routing: (Sender, TargetUsername, Hash/Body)
using OnMessageCallback = std::function<void(
    std::shared_ptr<ClientSession>, const std::string &, const std::string &)>;
// Callback for Nudge: (Sender, TargetUsername)
using OnNudgeCallback =
    std::function<void(std::shared_ptr<ClientSession>, const std::string &)>;
// New: Voice Message Callback (Sender, Recipient, Duration, Data)
using OnVoiceMessageCallback =
    std::function<void(std::shared_ptr<ClientSession>, const std::string &,
                       uint16_t, const std::vector<uint8_t> &)>;

using OnTypingIndicatorCallback = std::function<void(
    std::shared_ptr<ClientSession>, const std::string &, bool)>;
// Callback for Status: (Username) -> Status Int
using GetStatusCallback = std::function<int(const std::string &)>;
// Callback for Status Change: (Sender, NewStatus)
using OnStatusChangeCallback =
    std::function<void(std::shared_ptr<ClientSession>, int)>;

// Avatar Callbacks
using OnUpdateAvatarCallback = std::function<void(
    std::shared_ptr<ClientSession>, const std::vector<uint8_t> &)>;
using OnGetAvatarCallback =
    std::function<void(std::shared_ptr<ClientSession>, const std::string &)>;

// Game Status Callback (SenderSession, GameName, Score)
using OnGameStatusCallback = std::function<void(std::shared_ptr<ClientSession>,
                                                const std::string &, uint32_t)>;

// Callback for Disconnect
using OnDisconnectCallback = std::function<void(int)>;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
  // Pass Server pointer for Async Task dispatch, and Callbacks
  explicit ClientSession(
      int sessionId, asio::ip::tcp::socket socket,
      asio::ssl::context &sslContext, TcpServer *server,
      OnLoginCallback onLogin, OnMessageCallback onMessage,
      OnNudgeCallback onNudge, OnVoiceMessageCallback onVoiceMessage,
      OnTypingIndicatorCallback onTypingIndicator, GetStatusCallback getStatus,
      OnStatusChangeCallback onStatusChange,
      OnUpdateAvatarCallback onUpdateAvatar, OnGetAvatarCallback onGetAvatar,
      OnGameStatusCallback onGameStatus, OnDisconnectCallback onDisconnect);
  ~ClientSession(); // Closes socket if owned

  // Delete copy to prevent double-close of socket
  ClientSession(const ClientSession &) = delete;
  ClientSession &operator=(const ClientSession &) = delete;

  // No Move semantics when using shared_from_this
  ClientSession(ClientSession &&other) = delete;
  ClientSession &operator=(ClientSession &&other) = delete;

  int getId() const { return m_sessionId; }
  asio::ip::tcp::socket::lowest_layer_type &getSocket() {
    return m_socket.lowest_layer();
  }
  std::string getUsername() const { return m_username; }

  // High-level Send Helper (must become async)
  void sendPacket(const Packet &packet);

  // Start the asynchronous read loop
  void start();

  // Core Logic: Process incoming raw bytes
  void doRead();

private:
  // Helper to dispatch packets
  void onDataReceived(const char *data, size_t length);
  void processPacket(Packet &packet);

  // Handlers
  void handleLogin(Packet &packet);
  void handleRegister(Packet &packet);
  void handleDirectMessage(Packet &packet);
  void handleNudge(Packet &packet);
  void handleVoiceMessage(Packet &packet);
  void handleStatusChange(Packet &packet);
  void handleGameStatus(Packet &packet);

  // Contact Handlers
  void handleAddContact(Packet &packet);
  void handleRemoveContact(Packet &packet);

  // Avatar Handlers
  void handleUpdateAvatar(Packet &packet);
  void handleGetAvatar(Packet &packet);

private:
  int m_sessionId;
  asio::ssl::stream<asio::ip::tcp::socket> m_socket;
  std::string m_username;
  bool m_isLoggedIn;

  // Pointer to the Server for Async Task Queue access
  TcpServer *m_server;

  // Callbacks
  OnLoginCallback m_onLogin;
  OnMessageCallback m_onMessage;
  OnNudgeCallback m_onNudge;
  OnVoiceMessageCallback m_onVoiceMessage;
  OnTypingIndicatorCallback m_onTypingIndicator;
  GetStatusCallback m_getStatus;
  OnStatusChangeCallback m_onStatusChange;
  OnUpdateAvatarCallback m_onUpdateAvatar;
  OnGetAvatarCallback m_onGetAvatar;
  OnGameStatusCallback m_onGameStatus;
  OnDisconnectCallback m_onDisconnect;

  // Buffer for incoming partial data
  std::vector<uint8_t> m_buffer;

  // Outbound message queue to prevent overlapping async_writes on TLS stream
  std::deque<std::vector<uint8_t>> m_outbox;
  void doWrite();
};

} // namespace wizz
