#pragma once

#include "../common/Packet.h"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <cstdint>
#include <deque>
#include <functional> // For std::function
#include <memory>
#include <set>
#include <string>
#include <vector>

// Forward Declaration
namespace wizz {
class TcpServer;
}

namespace wizz {

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
  // Pass Server pointer for Async Task dispatch, and Callbacks
  explicit ClientSession(
      int sessionId, asio::ip::tcp::socket socket,
      asio::ssl::context &sslContext, TcpServer *server);
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
  void setUsername(const std::string& name) { m_username = name; }
  bool isLoggedIn() const { return m_isLoggedIn; }
  void setLoggedIn(bool b) { m_isLoggedIn = b; }
  TcpServer* getServer() const { return m_server; }

  // Contact list cache – populated at login, used for fast broadcasts
  void setContacts(std::set<std::string> contacts) { m_contacts = std::move(contacts); }
  const std::set<std::string>& getContacts() const { return m_contacts; }

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

  // Forward packets to router

private:
  int m_sessionId;
  asio::ssl::stream<asio::ip::tcp::socket> m_socket;
  std::string m_username;
  bool m_isLoggedIn;
  std::set<std::string> m_contacts; // cached at login

  // Pointer to the Server for Async Task Queue access
  TcpServer *m_server;



  // Buffer for incoming partial data
  std::vector<uint8_t> m_buffer;

  // Outbound message queue to prevent overlapping async_writes on TLS stream
  std::deque<std::vector<uint8_t>> m_outbox;
  void doWrite();
};

} // namespace wizz
