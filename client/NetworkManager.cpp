#include "NetworkManager.h"
#include <QDataStream>
#include <QDebug>

#include <QThread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

NetworkManager &NetworkManager::instance() {
  static NetworkManager *_instance = nullptr;
  if (!_instance) {
    QThread *thread = new QThread();
    _instance = new NetworkManager();
    _instance->moveToThread(thread);
    QObject::connect(thread, &QThread::started, _instance,
                     &NetworkManager::initSocket);
    thread->start();
  }
  return *_instance;
}

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
  qRegisterMetaType<wizz::Packet>("wizz::Packet");
  qRegisterMetaType<std::vector<uint8_t>>("std::vector<uint8_t>");
  qRegisterMetaType<uint16_t>("uint16_t");
}

void NetworkManager::initSocket() {
  m_socket = new QSslSocket(this);

  connect(m_socket, &QSslSocket::connected, this,
          &NetworkManager::onSocketConnected);
  connect(m_socket, &QSslSocket::disconnected, this,
          &NetworkManager::onSocketDisconnected);
  connect(m_socket, &QSslSocket::errorOccurred, this,
          &NetworkManager::onSocketError);
  connect(m_socket, &QSslSocket::readyRead, this, &NetworkManager::onReadyRead);

  // Ignore SSL errors since we use self-signed certificates for local
  // development
  connect(
      m_socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
      this, [this](const QList<QSslError> &errors) {
        qDebug() << "[Security] Ignoring SSL Errors since cert is self-signed:"
                 << errors;
        m_socket->ignoreSslErrors();
      });

  // Register Handlers is safe here on the background thread
  registerHandlers();
}

NetworkManager::~NetworkManager() {
  // Socket is child of this, processed automatically
}

void NetworkManager::connectToHost(const QString &host, quint16 port) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "connectToHost", Qt::QueuedConnection,
                              Q_ARG(QString, host), Q_ARG(quint16, port));
    return;
  }
  if (m_socket->state() != QAbstractSocket::UnconnectedState) {
    m_socket->disconnectFromHost();
  }
  m_socket->connectToHostEncrypted(host, port);
}

void NetworkManager::disconnectFromHost() {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "disconnectFromHost", Qt::QueuedConnection);
    return;
  }
  if (m_socket)
    m_socket->disconnectFromHost();
}

bool NetworkManager::isConnected() const { return m_isConnected.load(); }

void NetworkManager::sendPacket(const wizz::Packet &packet) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendPacket", Qt::QueuedConnection,
                              Q_ARG(wizz::Packet, packet));
    return;
  }
  if (!isConnected())
    return;

  std::vector<uint8_t> data = packet.serialize();
  m_socket->write(reinterpret_cast<const char *>(data.data()), data.size());
  m_socket->flush();
}

void NetworkManager::sendVoiceMessage(const QString &target, uint16_t duration,
                                      const std::vector<uint8_t> &data) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendVoiceMessage", Qt::QueuedConnection,
                              Q_ARG(QString, target), Q_ARG(uint16_t, duration),
                              Q_ARG(std::vector<uint8_t>, data));
    return;
  }
  wizz::Packet p(wizz::PacketType::VoiceMessage);
  p.writeString(target.toStdString());
  p.writeInt(static_cast<uint32_t>(duration));
  p.writeInt(static_cast<uint32_t>(data.size()));
  p.writeData(data.data(), data.size());

  sendPacket(p);
}

void NetworkManager::sendTypingPacket(const QString &target, bool isTyping) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendTypingPacket", Qt::QueuedConnection,
                              Q_ARG(QString, target), Q_ARG(bool, isTyping));
    return;
  }
  if (!isConnected())
    return;
  wizz::Packet p(wizz::PacketType::TypingIndicator);
  p.writeString(target.toStdString());
  p.writeInt(isTyping ? 1 : 0);
  sendPacket(p);
}

void NetworkManager::sendUpdateAvatar(const QByteArray &data) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendUpdateAvatar", Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));
    return;
  }
  if (!isConnected())
    return;
  wizz::Packet p(wizz::PacketType::UpdateAvatar);
  // Write size then data
  p.writeInt(static_cast<uint32_t>(data.size()));
  // access raw data
  p.writeData(reinterpret_cast<const uint8_t *>(data.data()), data.size());
  sendPacket(p);
}

void NetworkManager::requestAvatar(const QString &username) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "requestAvatar", Qt::QueuedConnection,
                              Q_ARG(QString, username));
    return;
  }
  if (!isConnected())
    return;
  wizz::Packet p(wizz::PacketType::GetAvatar);
  p.writeString(username.toStdString());
  sendPacket(p);
}

void NetworkManager::sendStatusChange(int status,
                                      const QString &statusMessage) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendStatusChange", Qt::QueuedConnection,
                              Q_ARG(int, status),
                              Q_ARG(QString, statusMessage));
    return;
  }
  if (!isConnected())
    return;

  wizz::Packet statusPkt(wizz::PacketType::ContactStatusChange);
  statusPkt.writeInt(static_cast<uint32_t>(status));
  // The server implementation currently does not read the status message string
  // statusPkt.writeString(statusMessage.toStdString());
  sendPacket(statusPkt);
}

void NetworkManager::sendGameStatus(const QString &gameName, uint32_t score) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendGameStatus", Qt::QueuedConnection,
                              Q_ARG(QString, gameName), Q_ARG(uint32_t, score));
    return;
  }
  if (!isConnected())
    return;

  wizz::Packet pkt(wizz::PacketType::GameStatus);
  pkt.writeString(gameName.toStdString());
  pkt.writeInt(score);
  sendPacket(pkt);
}

// --- Slots ---

void NetworkManager::onSocketConnected() {
  m_isConnected.store(true);
  emit connected();
}

void NetworkManager::onSocketDisconnected() {
  m_isConnected.store(false);
  emit disconnected();
}

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

      // Dispatch packet through registered handlers
      if (m_packetHandlers.contains(pkt.type())) {
        m_packetHandlers[pkt.type()](pkt);
      } else {
        // Unhandled packet logic can go here (or be ignored)
      }

    } catch (...) {
      // Log error?
      emit errorOccurred("Packet parsing error");
    }

    // 4. Remove from buffer
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + totalSize);
  }
}

// --- Packet Handlers ---

void NetworkManager::registerHandlers() {
  m_packetHandlers[wizz::PacketType::ContactList] = [this](wizz::Packet &pkt) {
    handleContactListPacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::ContactStatusChange] =
      [this](wizz::Packet &pkt) { handleContactStatusChangePacket(pkt); };
  m_packetHandlers[wizz::PacketType::Error] = [this](wizz::Packet &pkt) {
    handleErrorPacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::DirectMessage] =
      [this](wizz::Packet &pkt) { handleDirectMessagePacket(pkt); };
  m_packetHandlers[wizz::PacketType::Nudge] = [this](wizz::Packet &pkt) {
    handleNudgePacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::VoiceMessage] = [this](wizz::Packet &pkt) {
    handleVoiceMessagePacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::TypingIndicator] =
      [this](wizz::Packet &pkt) { handleTypingIndicatorPacket(pkt); };
  m_packetHandlers[wizz::PacketType::AvatarData] = [this](wizz::Packet &pkt) {
    handleAvatarDataPacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::GameStatus] = [this](wizz::Packet &pkt) {
    handleGameStatusPacket(pkt);
  };
}

void NetworkManager::handleContactListPacket(wizz::Packet &pkt) {
  uint32_t count = pkt.readInt();
  QList<QPair<QString, int>> contacts;
  for (uint32_t i = 0; i < count; ++i) {
    QString name = QString::fromStdString(pkt.readString());
    int status = static_cast<int>(pkt.readInt());
    contacts.append({name, status});
  }
  m_cachedContacts = contacts;
  emit contactListReceived(contacts);
}

void NetworkManager::handleContactStatusChangePacket(wizz::Packet &pkt) {
  int status = static_cast<int>(pkt.readInt());
  QString username = QString::fromStdString(pkt.readString());
  emit contactStatusChanged(username, status);
}

void NetworkManager::handleErrorPacket(wizz::Packet &pkt) {
  QString msg = QString::fromStdString(pkt.readString());
  emit errorOccurred(msg);
}

void NetworkManager::handleDirectMessagePacket(wizz::Packet &pkt) {
  QString sender = QString::fromStdString(pkt.readString());
  QString text = QString::fromStdString(pkt.readString());
  emit messageReceived(sender, text);
}

void NetworkManager::handleNudgePacket(wizz::Packet &pkt) {
  QString sender = QString::fromStdString(pkt.readString());
  emit nudgeReceived(sender);
}

void NetworkManager::handleVoiceMessagePacket(wizz::Packet &pkt) {
  QString sender = QString::fromStdString(pkt.readString());
  uint16_t duration = static_cast<uint16_t>(pkt.readInt());
  uint32_t len = pkt.readInt();
  // Safety / Sanity check
  if (len < 50 * 1024 * 1024) { // Limit to 50MB (arbitrary large for MVP)
    std::vector<uint8_t> audioData = pkt.readBytes(len);
    emit voiceMessageReceived(sender, duration, audioData);
  }
}

void NetworkManager::handleTypingIndicatorPacket(wizz::Packet &pkt) {
  QString sender = QString::fromStdString(pkt.readString());
  bool isTyping = (pkt.readInt() != 0);
  emit userTyping(sender, isTyping);
}

void NetworkManager::handleAvatarDataPacket(wizz::Packet &pkt) {
  QString username = QString::fromStdString(pkt.readString());
  uint32_t len = pkt.readInt();
  if (len < 10 * 1024 * 1024) { // 10MB limit
    std::vector<uint8_t> imgData = pkt.readBytes(len);
    QByteArray qData(reinterpret_cast<const char *>(imgData.data()),
                     imgData.size());
    emit avatarReceived(username, qData);
  }
}

void NetworkManager::handleGameStatusPacket(wizz::Packet &pkt) {
  // Server will relay: username(string) -> gameName(string) -> score(uint32_t)
  QString username = QString::fromStdString(pkt.readString());
  QString gameName = QString::fromStdString(pkt.readString());
  uint32_t score = pkt.readInt();
  emit gameStatusChanged(username, gameName, score);
}
