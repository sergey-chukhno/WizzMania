#pragma once

#include "../common/Types.h" // For SocketType
#include <string>
#include <vector>

namespace wizz {

class ClientSession {
public:
  explicit ClientSession(SocketType socket);
  ~ClientSession(); // Closes socket if owned

  // Delete copy to prevent double-close of socket
  ClientSession(const ClientSession &) = delete;
  ClientSession &operator=(const ClientSession &) = delete;

  // Allow Move (transfer ownership)
  ClientSession(ClientSession &&other) noexcept;
  ClientSession &operator=(ClientSession &&other) noexcept;

  SocketType getSocket() const { return m_socket; }

private:
  SocketType m_socket;
  std::string m_username;
  bool m_isLoggedIn;

  // Buffer for incoming partial data
  std::vector<uint8_t> m_buffer;
};

} // namespace wizz
