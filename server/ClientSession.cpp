#include "ClientSession.h"
#include <utility> // for std::move

namespace wizz {

ClientSession::ClientSession(SocketType socket)
    : m_socket(socket), m_isLoggedIn(false) {}

ClientSession::~ClientSession() {
  if (m_socket != INVALID_SOCKET_VAL) {
    close_socket_raw(m_socket);
  }
}

// Move Constructor
ClientSession::ClientSession(ClientSession &&other) noexcept
    : m_socket(other.m_socket), m_username(std::move(other.m_username)),
      m_isLoggedIn(other.m_isLoggedIn), m_buffer(std::move(other.m_buffer)) {
  // STEAL ownership: clear the other so it doesn't close the socket
  other.m_socket = INVALID_SOCKET_VAL;
}

// Move Assignment
ClientSession &ClientSession::operator=(ClientSession &&other) noexcept {
  if (this != &other) {
    // 1. Clean up our own resource if we have one
    if (m_socket != INVALID_SOCKET_VAL) {
      close_socket_raw(m_socket);
    }

    // 2. Steal resources
    m_socket = other.m_socket;
    m_username = std::move(other.m_username);
    m_isLoggedIn = other.m_isLoggedIn;
    m_buffer = std::move(other.m_buffer);

    // 3. Nullify other
    other.m_socket = INVALID_SOCKET_VAL;
  }
  return *this;
}

} // namespace wizz
