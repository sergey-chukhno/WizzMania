#pragma once

#include "../common/Packet.h"
#include "../common/Types.h" // For SocketType
#include <string>
#include <vector>

// Forward Declaration
namespace wizz {
class DatabaseManager;
}

namespace wizz {

class ClientSession {
public:
  // Pass DB by reference, store as pointer
  explicit ClientSession(SocketType socket, DatabaseManager &db);
  ~ClientSession(); // Closes socket if owned

  // Delete copy to prevent double-close of socket
  ClientSession(const ClientSession &) = delete;
  ClientSession &operator=(const ClientSession &) = delete;

  // Allow Move (transfer ownership)
  ClientSession(ClientSession &&other) noexcept;
  ClientSession &operator=(ClientSession &&other) noexcept;

  SocketType getSocket() const { return m_socket; }

  // Core Logic: Process incoming raw bytes
  // Returns false if the session should be closed (DoS/Error)
  bool onDataReceived(const char *data, size_t length);

private:
  // Helper to dispatch packets
  void processPacket(Packet &packet);

  // Handlers
  void handleLogin(Packet &packet);
  void handleRegister(Packet &packet);

private:
  SocketType m_socket;
  std::string m_username;
  bool m_isLoggedIn;

  // Pointer to the Shared Database (Owned by TcpServer)
  DatabaseManager *m_db;

  // Buffer for incoming partial data
  std::vector<uint8_t> m_buffer;
};

} // namespace wizz
