#pragma once

#include "../common/Packet.h"
#include "../common/Types.h" // For SocketType
#include <string>
#include <vector>

#include <functional> // For std::function

// Forward Declaration
namespace wizz {
class DatabaseManager;
}

namespace wizz {

class ClientSession; // Forward decl
using OnLoginCallback = std::function<void(ClientSession *)>;
// Callback for Routing: (Sender, TargetUsername, Hash/Body)
using OnMessageCallback = std::function<void(
    ClientSession *, const std::string &, const std::string &)>;
// Callback for Nudge: (Sender, TargetUsername)
using OnNudgeCallback =
    std::function<void(ClientSession *, const std::string &)>;
// New: Voice Message Callback (Sender, Recipient, Duration, Data)
using OnVoiceMessageCallback =
    std::function<void(ClientSession *, const std::string &, uint16_t,
                       const std::vector<uint8_t> &)>;

using OnTypingIndicatorCallback =
    std::function<void(ClientSession *, const std::string &, bool)>;
// Callback for Status: (Username) -> Status Int
using GetStatusCallback = std::function<int(const std::string &)>;
// Callback for Status Change: (Sender, NewStatus)
using OnStatusChangeCallback = std::function<void(ClientSession *, int)>;

class ClientSession {
public:
  // Pass DB by reference, store as pointer, and Callbacks
  explicit ClientSession(SocketType socket, DatabaseManager &db,
                         OnLoginCallback onLogin, OnMessageCallback onMessage,
                         OnNudgeCallback onNudge,
                         OnVoiceMessageCallback onVoiceMessage,
                         OnTypingIndicatorCallback onTypingIndicator,
                         GetStatusCallback getStatus,
                         OnStatusChangeCallback onStatusChange);
  ~ClientSession(); // Closes socket if owned

  // Delete copy to prevent double-close of socket
  ClientSession(const ClientSession &) = delete;
  ClientSession &operator=(const ClientSession &) = delete;

  // Allow Move (transfer ownership)
  ClientSession(ClientSession &&other) noexcept;
  ClientSession &operator=(ClientSession &&other) noexcept;

  SocketType getSocket() const { return m_socket; }
  std::string getUsername() const { return m_username; }

  // High-level Send Helper
  void sendPacket(const Packet &packet);

  // Core Logic: Process incoming raw bytes
  // Returns false if the session should be closed (DoS/Error)
  bool onDataReceived(const char *data, size_t length);

private:
  // Helper to dispatch packets
  void processPacket(Packet &packet);

  // Handlers
  void handleLogin(Packet &packet);
  void handleRegister(Packet &packet);
  void handleDirectMessage(Packet &packet);
  void handleNudge(Packet &packet);
  void handleVoiceMessage(Packet &packet);
  void handleStatusChange(Packet &packet);

  // Contact Handlers
  void handleAddContact(Packet &packet);
  void handleRemoveContact(Packet &packet);

private:
  SocketType m_socket;
  std::string m_username;
  bool m_isLoggedIn;

  // Pointer to the Shared Database (Owned by TcpServer)
  DatabaseManager *m_db;

  // Callbacks
  OnLoginCallback m_onLogin;
  OnMessageCallback m_onMessage;
  OnNudgeCallback m_onNudge;
  OnVoiceMessageCallback m_onVoiceMessage;
  OnTypingIndicatorCallback m_onTypingIndicator;
  GetStatusCallback m_getStatus;
  OnStatusChangeCallback m_onStatusChange;

  // Buffer for incoming partial data
  std::vector<uint8_t> m_buffer;
};

} // namespace wizz
