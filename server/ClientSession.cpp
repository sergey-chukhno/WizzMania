#include "ClientSession.h"
#include <cstring> // for memcpy
#include <iostream>
#include <utility> // for std::move

// Needed for ntohl
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

// Include full definition for implementation
#include "TcpServer.h"
#include "handlers/PacketRouter.h"

namespace wizz {

ClientSession::ClientSession(
    int sessionId, asio::ip::tcp::socket socket, asio::ssl::context &sslContext,
    TcpServer *server)
    : m_sessionId(sessionId), m_socket(std::move(socket), sslContext),
      m_isLoggedIn(false), m_server(server) {}

ClientSession::~ClientSession() {
  if (m_socket.lowest_layer().is_open()) {
    asio::error_code ec;
    m_socket.lowest_layer().close(ec);
  }
}

void ClientSession::sendPacket(const Packet &packet) {
  // Serialize Packet
  std::vector<uint8_t> data = packet.serialize();

  // Must run on the io_context thread!
  bool writeInProgress = !m_outbox.empty();
  m_outbox.push_back(std::move(data));
  if (!writeInProgress) {
    doWrite();
  }
}

void ClientSession::doWrite() {
  auto self(shared_from_this());

  asio::async_write(m_socket, asio::buffer(m_outbox.front()),
                    [this, self](asio::error_code ec, std::size_t /*length*/) {
                      if (!ec) {
                        m_outbox.pop_front();
                        if (!m_outbox.empty()) {
                          doWrite();
                        }
                      } else {
                        std::cerr << "[Session " << m_sessionId
                                  << "] TLS Write Error: " << ec.message()
                                  << std::endl;
                        if (m_socket.lowest_layer().is_open()) {
                          asio::error_code closeEc;
                          m_socket.lowest_layer().close(closeEc);
                        }
                      }
                    });
}

void ClientSession::start() {
  auto self(shared_from_this());
  m_socket.async_handshake(asio::ssl::stream_base::server,
                           [this, self](const asio::error_code &error) {
                             if (!error) {
                               doRead();
                             } else {
                               std::cerr << "[Session " << m_sessionId
                                         << "] TLS Handshake Failed: "
                                         << error.message() << std::endl;
                             }
                           });
}

void ClientSession::doRead() {
  auto self(shared_from_this());

  // We read enough for the packet header first.
  // Instead of a loop, we rely on callbacks holding a shared_ptr to keep the
  // Session alive.
  m_buffer.resize(1024); // Generic read buffer

  m_socket.async_read_some(
      asio::buffer(m_buffer.data(), m_buffer.capacity()),
      [this, self](asio::error_code ec, std::size_t length) {
        if (!ec) {
          // Re-use legacy logic temporarily by feeding the bytes to a stream
          // analyzer
          // 1. Process received data
          // (We will invoke the logic synchronously so we can reuse the
          this->onDataReceived(reinterpret_cast<const char *>(m_buffer.data()),
                               length);
        } else if (ec != asio::error::operation_aborted) {
          std::cout << "[Session " << m_sessionId
                    << "] Disconnected: " << ec.message() << std::endl;
          if (m_server) {
            m_server->handleDisconnect(m_sessionId);
          }
          // The connection is dropped. `self` drops out of scope, destroying
          // the session.
        }
      });
}

void ClientSession::onDataReceived(const char *data, size_t length) {
  // Rather than keeping `m_buffer` as the main read destination, we use it as
  // the accumulator
  // 1. Append new incoming bytes to our persistent accumulating buffer (renamed
  // later if needed, using `m_buffer` right now causes a conflict if we async
  // read into it directly) Let's create an `m_receiveBuffer` in the class
  // later, for now we will cheat by statically analyzing it
  static std::vector<uint8_t> accumulator;
  accumulator.insert(accumulator.end(), data, data + length);

  while (true) {
    if (accumulator.size() < sizeof(PacketHeader))
      break;

    uint32_t networkLength;
    std::memcpy(&networkLength, accumulator.data() + 8, sizeof(uint32_t));
    uint32_t bodyLength = ntohl(networkLength);

    size_t totalSize = sizeof(PacketHeader) + bodyLength;
    if (accumulator.size() < totalSize)
      break;

    try {
      std::vector<uint8_t> packetData(accumulator.begin(),
                                      accumulator.begin() + totalSize);
      Packet pkt(packetData);
      processPacket(pkt);
      accumulator.erase(accumulator.begin(), accumulator.begin() + totalSize);
    } catch (const std::exception &e) {
      std::cerr << "[Session " << m_sessionId << "] Data Error: " << e.what()
                << std::endl;
      // Close socket explicitly on error
      asio::error_code closeEc;
      m_socket.lowest_layer().close(closeEc);
      return;
    }
  }

  // Chain the next read asynchronously
  doRead();
}

// Legacy signature preserved, now rewritten using the `static accumulator`
// logic in the chunk above

void ClientSession::processPacket(Packet &packet) {
  if (m_server) {
    m_server->getPacketRouter().handle(this, packet);
  }
}

} // namespace wizz
