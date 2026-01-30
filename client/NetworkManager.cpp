#include "NetworkManager.h"
#include <QDataStream>
#include <QDebug>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

NetworkManager &NetworkManager::instance() {
  static NetworkManager _instance;
  return _instance;
}

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
  m_socket = new QTcpSocket(this);

  connect(m_socket, &QTcpSocket::connected, this,
          &NetworkManager::onSocketConnected);
  connect(m_socket, &QTcpSocket::disconnected, this,
          &NetworkManager::onSocketDisconnected);
  // Note: QOverload is needed for errorOccurred because it's overloaded in
  // older Qt versions, but in Qt6 errorOccurred is standard.
  connect(m_socket, &QTcpSocket::errorOccurred, this,
          &NetworkManager::onSocketError);
  connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
}

NetworkManager::~NetworkManager() {
  // Socket is child of this, processed automatically
}

void NetworkManager::connectToHost(const QString &host, quint16 port) {
  if (m_socket->state() != QAbstractSocket::UnconnectedState) {
    m_socket->disconnectFromHost();
  }
  m_socket->connectToHost(host, port);
}

void NetworkManager::disconnectFromHost() { m_socket->disconnectFromHost(); }

bool NetworkManager::isConnected() const {
  return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendPacket(const wizz::Packet &packet) {
  if (!isConnected())
    return;

  std::vector<uint8_t> data = packet.serialize();
  m_socket->write(reinterpret_cast<const char *>(data.data()), data.size());
  m_socket->flush();
}

// --- Slots ---

void NetworkManager::onSocketConnected() { emit connected(); }

void NetworkManager::onSocketDisconnected() { emit disconnected(); }

void NetworkManager::onSocketError(QAbstractSocket::SocketError socketError) {
  Q_UNUSED(socketError);
  emit errorOccurred(m_socket->errorString());
}

void NetworkManager::onReadyRead() {
  QByteArray newData = m_socket->readAll();
  m_buffer.insert(m_buffer.end(), newData.begin(), newData.end());

  // Process Buffer (Loop for multiple packets)
  while (true) {
    // 1. Check Header
    if (m_buffer.size() < 12)
      break; // Not enough for header

    // Read Body Length (Offset 8)
    uint32_t netLen;
    std::memcpy(&netLen, m_buffer.data() + 8, 4);
    uint32_t bodyLen =
        ntohl(netLen); // Helper needed? ntohl is cross-platform usually

    // 2. Check Full Packet
    size_t totalSize = 12 + bodyLen;
    if (m_buffer.size() < totalSize)
      break; // Wait for more data

    // 3. Extract
    std::vector<uint8_t> packetData(m_buffer.begin(),
                                    m_buffer.begin() + totalSize);

    try {
      wizz::Packet pkt(packetData);
      emit packetReceived(pkt);

      // Parse high-level packets
      // Parse high-level packets
      if (pkt.type() == wizz::PacketType::ContactList) {
        uint32_t count = pkt.readInt();
        QList<QPair<QString, int>> contacts;
        for (uint32_t i = 0; i < count; ++i) {
          QString name = QString::fromStdString(pkt.readString());
          int status = static_cast<int>(pkt.readInt());
          contacts.append({name, status});
        }
        emit contactListReceived(contacts);
      } else if (pkt.type() == wizz::PacketType::ContactStatusChange) {
        int status = static_cast<int>(pkt.readInt());
        QString username = QString::fromStdString(pkt.readString());
        emit contactStatusChanged(username, status);
      } else if (pkt.type() == wizz::PacketType::Error) {
        QString msg = QString::fromStdString(pkt.readString());
        emit errorOccurred(msg);
      } else if (pkt.type() == wizz::PacketType::DirectMessage) {
        QString sender = QString::fromStdString(pkt.readString());
        QString text = QString::fromStdString(pkt.readString());
        emit messageReceived(sender, text);
      } else if (pkt.type() == wizz::PacketType::Nudge) {
        QString sender = QString::fromStdString(pkt.readString());
        emit nudgeReceived(sender);
      }

    } catch (...) {
      // Log error?
      emit errorOccurred("Packet parsing error");
    }

    // 4. Remove from buffer
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + totalSize);
  }
}
