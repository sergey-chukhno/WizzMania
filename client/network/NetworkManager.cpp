#include "NetworkManager.h"
#include "../crypto/SignalProvider.h"
#include "../crypto/SignalStoreContext.h"
#include <signal_protocol.h>
#include <session_builder.h>
#include <session_cipher.h>
#include <protocol.h>
#include <curve.h>
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
    _instance->m_thread = thread;
    _instance->moveToThread(thread);

    // Ensure the socket is properly deleted via event loop later
    QObject::connect(_instance, &NetworkManager::shutdownRequested, _instance,
                     [=]() {
                       if (_instance->m_storeContext) {
                           signal_protocol_store_context_destroy(_instance->m_storeContext);
                           _instance->m_storeContext = nullptr;
                       }
                       if (_instance->m_signalContext) {
                           // No destroy func exposed cleanly by context
                       }
                       if (_instance->m_localDb) {
                           delete _instance->m_localDb;
                           _instance->m_localDb = nullptr;
                       }
                       if (_instance->m_socket) {
                         _instance->m_socket->disconnectFromHost();
                         _instance->m_socket->deleteLater();
                         _instance->m_socket = nullptr;
                       }
                       thread->quit();
                     });

    QObject::connect(thread, &QThread::started, _instance,
                     &NetworkManager::initSocket);
    thread->start();
  }
  return *_instance;
}

void NetworkManager::shutdown() {
  static NetworkManager *_instance = &instance();
  if (_instance && _instance->m_thread) {
    qDebug() << "[Network] Initiating shutdown...";
    emit _instance->shutdownRequested();
    _instance->m_thread->quit(); 

    if (!_instance->m_thread->wait(2000)) {
      qDebug() << "[Network] Shutdown timeout! Forcing socket abort.";
      if (_instance->m_socket) {
        _instance->m_socket->abort();
      }
      _instance->m_thread->terminate();
      _instance->m_thread->wait(1000);
    }
    qDebug() << "[Network] Shutdown complete.";
  }
}

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
  qRegisterMetaType<wizz::Packet>("wizz::Packet");
  qRegisterMetaType<std::vector<uint8_t>>("std::vector<uint8_t>");
  qRegisterMetaType<uint16_t>("uint16_t");
  qRegisterMetaType<QList<std::tuple<QString, int, QString>>>(
      "QList<std::tuple<QString, int, QString>>");
}

void NetworkManager::initSocket() {
  m_socket = new QSslSocket(this);

  // Initialize E2EE Store
  try {
      m_localDb = new wizz::client::LocalDatabase("wizz_session.db");
      wizz::client::crypto::setupSignalCryptoProvider(&m_signalContext);
      wizz::client::crypto::setupSignalStoreContext(&m_storeContext, m_signalContext, m_localDb);
      wizz::client::crypto::initializeLocalKeys(m_signalContext, m_localDb);
      qDebug() << "[Security] Signal E2EE Provider and Key Storage mounted successfully.";
  } catch (const std::exception& e) {
      qWarning() << "[Security] Failed to initialize E2EE Store:" << e.what();
  }

  connect(m_socket, &QSslSocket::connected, this,
          &NetworkManager::onSocketConnected);
  connect(m_socket, &QSslSocket::disconnected, this,
          &NetworkManager::onSocketDisconnected);
  connect(m_socket, &QSslSocket::errorOccurred, this,
          &NetworkManager::onSocketError);
  connect(m_socket, &QSslSocket::readyRead, this, &NetworkManager::onReadyRead);

  // Ignore SSL errors for local dev
  connect(
      m_socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
      this, [this](const QList<QSslError> &errors) {
        qDebug() << "[Security] Ignoring SSL Errors since cert is self-signed:"
                 << errors;
        m_socket->ignoreSslErrors();
      });

  registerHandlers();
  emit initialized();
}

NetworkManager::~NetworkManager() {
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

void NetworkManager::uploadStoredPreKeys() {
    if (!m_localDb || !isConnected()) return;

    wizz::Packet pkt(wizz::PacketType::UploadPreKeys);

    // 1. Identity Key Public
    std::vector<uint8_t> identBuffer;
    uint32_t regId = 0;
    m_localDb->getIdentity(identBuffer, regId);
    if (!identBuffer.empty()) {
        uint8_t pubLen = identBuffer[0];
        pkt.writeString(std::string(reinterpret_cast<char*>(&identBuffer[1]), pubLen));
    } else {
        pkt.writeString("");
    }

    // 2. Signed PreKey Public & Signature
    std::vector<uint8_t> signedPrekeyBuf;
    if (m_localDb->loadSignedPreKey(1, signedPrekeyBuf) && signedPrekeyBuf.size() > 0) {
        uint8_t pubLen = signedPrekeyBuf[0];
        pkt.writeString(std::string(reinterpret_cast<char*>(&signedPrekeyBuf[1]), pubLen));
        
        size_t offset = 1 + pubLen + 32;
        if (offset < signedPrekeyBuf.size()) {
            uint8_t sigLen = signedPrekeyBuf[offset];
            pkt.writeString(std::string(reinterpret_cast<char*>(&signedPrekeyBuf[offset + 1]), sigLen));
        } else {
            pkt.writeString("");
        }
    } else {
        pkt.writeString("");
        pkt.writeString("");
    }

    // 3. PreKeys (OTKs)
    pkt.writeInt(100); 
    for (uint32_t i = 1; i <= 100; i++) {
        std::vector<uint8_t> prekeyBuf;
        if (m_localDb->loadPreKey(i, prekeyBuf) && prekeyBuf.size() > 0) {
            uint8_t pubLen = prekeyBuf[0];
            pkt.writeInt(i);
            pkt.writeString(std::string(reinterpret_cast<char*>(&prekeyBuf[1]), pubLen));
        }
    }

    sendPacket(pkt);
    qDebug() << "[Security] Signal Identity Key & PreKey Bundle pushed to server!";
}

void NetworkManager::sendEncryptedMessage(const QString &target, const QString &text) {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "sendEncryptedMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, target), Q_ARG(QString, text));
        return;
    }

    if (!isConnected()) return;

    std::string targetStr = target.toStdString();
    signal_protocol_address address = { targetStr.c_str(), targetStr.length(), 1 };
    
    session_cipher *cipher = nullptr;
    int res = session_cipher_create(&cipher, m_storeContext, &address, m_signalContext);
    if (res < 0) {
        qWarning() << "[Security] Failed to create Session Cipher for" << target;
        return;
    }

    ciphertext_message *encrypted = nullptr;
    QByteArray plainData = text.toUtf8();
    res = session_cipher_encrypt(cipher, reinterpret_cast<const uint8_t*>(plainData.constData()), plainData.size(), &encrypted);

    if (res == -1004) { 
        qDebug() << "[Security] No session with" << target << "- fetching PreKey Bundle...";
        m_pendingMessages[target].append(text);
        
        wizz::Packet fetchPkt(wizz::PacketType::FetchPreKeyBundle);
        fetchPkt.writeString(targetStr);
        sendPacket(fetchPkt);
    } else if (res == 0) {
        signal_buffer *serialized = ciphertext_message_get_serialized(encrypted);
        
        wizz::Packet e2ePkt(wizz::PacketType::E2EMessage);
        e2ePkt.writeString(targetStr);
        e2ePkt.writeInt(static_cast<uint32_t>(signal_buffer_len(serialized)));
        e2ePkt.writeData(signal_buffer_data(serialized), signal_buffer_len(serialized));
        sendPacket(e2ePkt);
        
        SIGNAL_UNREF(encrypted);
    } else {
        qWarning() << "[Security] Encryption failed for" << target << "Result:" << res;
    }

    session_cipher_free(cipher);
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
  p.writeInt(static_cast<uint32_t>(data.size()));
  p.writeData(reinterpret_cast<const uint8_t *>(data.data()), data.size());
  sendPacket(p);
}

void NetworkManager::sendUpdateStatus(const QString &status) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendUpdateStatus", Qt::QueuedConnection,
                              Q_ARG(QString, status));
    return;
  }
  if (!isConnected())
    return;

  wizz::Packet p(wizz::PacketType::UpdateStatus);
  p.writeString(status.toStdString());
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

void NetworkManager::sendGameInvite(const QString &target,
                                    const QString &gameName) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendGameInvite", Qt::QueuedConnection,
                              Q_ARG(QString, target), Q_ARG(QString, gameName));
    return;
  }
  if (!isConnected())
    return;

  wizz::Packet pkt(wizz::PacketType::GameInvite);
  pkt.writeString(target.toStdString());
  pkt.writeString(gameName.toStdString());
  sendPacket(pkt);
}

void NetworkManager::sendGameInviteResponse(const QString &originalSender,
                                             const QString &gameName,
                                             bool accepted) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendGameInviteResponse",
                              Qt::QueuedConnection,
                              Q_ARG(QString, originalSender),
                              Q_ARG(QString, gameName), Q_ARG(bool, accepted));
    return;
  }
  if (!isConnected())
    return;

  wizz::Packet pkt(wizz::PacketType::GameInviteResponse);
  pkt.writeString(originalSender.toStdString());
  pkt.writeString(gameName.toStdString());
  pkt.writeInt(accepted ? 1 : 0);
  sendPacket(pkt);
}

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

  while (true) {
    if (m_buffer.size() < 12)
      break; 

    uint32_t netLen;
    std::memcpy(&netLen, m_buffer.data() + 8, 4);
    uint32_t bodyLen = ntohl(netLen); 

    size_t totalSize = 12 + bodyLen;
    if (m_buffer.size() < totalSize)
      break; 

    std::vector<uint8_t> packetData(m_buffer.begin(),
                                    m_buffer.begin() + totalSize);

    try {
      wizz::Packet pkt(packetData);
      processPacket(pkt);
    } catch (...) {
      emit errorOccurred("Packet parsing error");
    }

    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + totalSize);
  }
}

void NetworkManager::processPacket(wizz::Packet &pkt) {
  emit packetReceived(pkt);

  if (m_packetHandlers.contains(pkt.type())) {
    m_packetHandlers[pkt.type()](pkt);
  }
}

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
  m_packetHandlers[wizz::PacketType::GameInvite] = [this](wizz::Packet &pkt) {
    handleGameInvitePacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::GameInviteResponse] =
      [this](wizz::Packet &pkt) { handleGameInviteResponsePacket(pkt); };
  m_packetHandlers[wizz::PacketType::GameStart] = [this](wizz::Packet &pkt) {
    handleGameStartPacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::GameMove] = [this](wizz::Packet &pkt) {
    handleGameMovePacket(pkt);
  };
  m_packetHandlers[wizz::PacketType::PreKeyBundleResponse] = [this](wizz::Packet &pkt) {
    handlePreKeyBundleResponse(pkt);
  };
  m_packetHandlers[wizz::PacketType::E2EMessage] = [this](wizz::Packet &pkt) {
    handleE2EMessagePacket(pkt);
  };
}

void NetworkManager::handleContactListPacket(wizz::Packet &pkt) {
  uint32_t count = pkt.readInt();
  QList<std::tuple<QString, int, QString>> contacts;
  for (uint32_t i = 0; i < count; ++i) {
    QString name = QString::fromStdString(pkt.readString());
    int status = static_cast<int>(pkt.readInt());
    QString statusMsg = QString::fromStdString(pkt.readString());
    contacts.append(std::make_tuple(name, status, statusMsg));
  }
  m_cachedContacts = contacts;
  emit contactListReceived(contacts);
}

void NetworkManager::handleContactStatusChangePacket(wizz::Packet &pkt) {
  int status = static_cast<int>(pkt.readInt());
  QString username = QString::fromStdString(pkt.readString());
  QString statusMsg = QString::fromStdString(pkt.readString());
  emit contactStatusChanged(username, status, statusMsg);
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
  if (len < 50 * 1024 * 1024) { 
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
  if (len < 10 * 1024 * 1024) { 
    std::vector<uint8_t> imgData = pkt.readBytes(len);
    QByteArray qData(reinterpret_cast<const char *>(imgData.data()),
                     imgData.size());
    emit avatarReceived(username, qData);
  }
}

void NetworkManager::handlePreKeyBundleResponse(wizz::Packet &pkt) {
    QString target = QString::fromStdString(pkt.readString());
    std::string identRaw = pkt.readString();
    std::string signedPreKeyRaw = pkt.readString();
    std::string signatureRaw = pkt.readString();
    uint32_t otkId = pkt.readInt();
    std::string otkRaw = pkt.readString();

    ec_public_key *identKey = nullptr;
    ec_public_key *signedPreKey = nullptr;
    ec_public_key *otk = nullptr;

    curve_decode_point(&identKey, reinterpret_cast<const uint8_t*>(identRaw.data()), identRaw.size(), m_signalContext);
    curve_decode_point(&signedPreKey, reinterpret_cast<const uint8_t*>(signedPreKeyRaw.data()), signedPreKeyRaw.size(), m_signalContext);
    if (!otkRaw.empty()) {
        curve_decode_point(&otk, reinterpret_cast<const uint8_t*>(otkRaw.data()), otkRaw.size(), m_signalContext);
    }

    session_pre_key_bundle *bundle = nullptr;
    session_pre_key_bundle_create(&bundle, 0, 1, otkId, otk, 1, signedPreKey, 
                                   reinterpret_cast<const uint8_t*>(signatureRaw.data()), signatureRaw.size(), identKey);

    std::string targetStr = target.toStdString();
    signal_protocol_address address = { targetStr.c_str(), targetStr.length(), 1 };
    
    session_builder *builder = nullptr;
    session_builder_create(&builder, m_storeContext, &address, m_signalContext);
    int res = session_builder_process_pre_key_bundle(builder, bundle);

    if (res == 0) {
        QStringList pending = m_pendingMessages.take(target);
        for (const QString &text : pending) {
            sendEncryptedMessage(target, text);
        }
    } 

    session_builder_free(builder);
    session_pre_key_bundle_destroy(reinterpret_cast<signal_type_base*>(bundle));
    SIGNAL_UNREF(identKey);
    SIGNAL_UNREF(signedPreKey);
    if (otk) SIGNAL_UNREF(otk);
}

void NetworkManager::handleE2EMessagePacket(wizz::Packet &pkt) {
    QString sender = QString::fromStdString(pkt.readString());
    uint32_t len = pkt.readInt();
    std::vector<uint8_t> data = pkt.readBytes(len);

    std::string senderStr = sender.toStdString();
    signal_protocol_address address = { senderStr.c_str(), senderStr.length(), 1 };

    session_cipher *cipher = nullptr;
    session_cipher_create(&cipher, m_storeContext, &address, m_signalContext);

    signal_buffer *plaintext = nullptr;
    
    int res = -1;
    if (data[0] == CIPHERTEXT_PREKEY_TYPE) {
        pre_key_signal_message *m = nullptr;
        pre_key_signal_message_deserialize(&m, data.data(), data.size(), m_signalContext);
        res = session_cipher_decrypt_pre_key_signal_message(cipher, m, nullptr, &plaintext);
        SIGNAL_UNREF(m);
    } else {
        signal_message *m = nullptr;
        signal_message_deserialize(&m, data.data(), data.size(), m_signalContext);
        res = session_cipher_decrypt_signal_message(cipher, m, nullptr, &plaintext);
        SIGNAL_UNREF(m);
    }

    if (res == 0) {
        QString text = QString::fromUtf8(reinterpret_cast<const char*>(signal_buffer_data(plaintext)), signal_buffer_len(plaintext));
        emit messageReceived(sender, text);
    }

    if (plaintext) signal_buffer_free(plaintext);
    session_cipher_free(cipher);
}

void NetworkManager::handleGameStatusPacket(wizz::Packet &pkt) {
  QString username = QString::fromStdString(pkt.readString());
  QString gameName = QString::fromStdString(pkt.readString());
  uint32_t score = pkt.readInt();
  emit gameStatusChanged(username, gameName, score);
}

void NetworkManager::handleGameInvitePacket(wizz::Packet &pkt) {
  QString sender = QString::fromStdString(pkt.readString());
  QString gameName = QString::fromStdString(pkt.readString());
  emit gameInviteReceived(sender, gameName);
}

void NetworkManager::handleGameInviteResponsePacket(wizz::Packet &pkt) {
  QString originalTarget = QString::fromStdString(pkt.readString());
  QString gameName = QString::fromStdString(pkt.readString());
  bool accepted = (pkt.readInt() != 0);
  emit gameInviteResponseReceived(originalTarget, gameName, accepted);
}

void NetworkManager::handleGameStartPacket(wizz::Packet &pkt) {
  QString gameName = QString::fromStdString(pkt.readString());
  QString roomId = QString::fromStdString(pkt.readString());
  char symbol = static_cast<char>(pkt.readInt());
  QString opponent = QString::fromStdString(pkt.readString());
  emit gameStartReceived(gameName, roomId, symbol, opponent);
}

void NetworkManager::sendGameMove(const QString &roomId, uint8_t cellIndex) {
  if (QThread::currentThread() != this->thread()) {
    QMetaObject::invokeMethod(this, "sendGameMove", Qt::QueuedConnection,
                              Q_ARG(QString, roomId),
                              Q_ARG(uint8_t, cellIndex));
    return;
  }
  if (!isConnected())
    return;
  wizz::Packet p(wizz::PacketType::GameMove);
  p.writeString(roomId.toStdString());
  p.writeInt(static_cast<uint32_t>(cellIndex));
  sendPacket(p);
}

void NetworkManager::handleGameMovePacket(wizz::Packet &pkt) {
  QString roomId = QString::fromStdString(pkt.readString());
  uint8_t cellIndex = static_cast<uint8_t>(pkt.readInt());
  emit gameMoveReceived(roomId, cellIndex);
}
