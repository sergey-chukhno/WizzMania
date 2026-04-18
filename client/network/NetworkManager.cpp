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
#include <iostream>
#include <QDebug>
#include <iomanip>
#include <sstream>

static QString normalizeUsername(const QString& name) {
    return name.trimmed().toLower().remove('\"').remove('\'');
}

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

  // E2EE initialization is now handled per-session in initializeE2EE()

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
 
const signal_protocol_address* NetworkManager::getSignalAddress(const QString& username) {
    QString normalized = normalizeUsername(username);
    if (!m_addressRegistry.contains(normalized)) {
        auto entry = std::make_shared<AddressEntry>();
        entry->name = normalized.toStdString();
        entry->address.name = entry->name.c_str();
        entry->address.name_len = entry->name.length();
        entry->address.device_id = 1;
        m_addressRegistry[normalized] = std::move(entry);
    }
    return &m_addressRegistry[normalized]->address;
}

void NetworkManager::initializeE2EE(const QString &username) {
    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    
    QString dbPathStr = normalizeUsername(username) + "_session.db";
    std::string dbPath = dbPathStr.toStdString();

    // IDEMPOTENCY: If already initialized for THIS user, don't touch anything.
    if (m_e2eeInitialized && m_localDb && m_localDb->getDbPath() == dbPath) {
        return;
    }

    qDebug() << "[Security] Initializing isolated E2EE store for" << username << "at" << dbPathStr;

    try {
        // --- CLEANUP ---
        if (m_storeContext) {
            signal_protocol_store_context_destroy(m_storeContext);
            m_storeContext = nullptr;
        }
        if (m_signalContext) {
            signal_context_destroy(m_signalContext);
            m_signalContext = nullptr;
        }
        if (m_localDb) {
            delete m_localDb;
            m_localDb = nullptr;
        }
        
        // NOTE: We no longer clear m_addressRegistry here. 
        // Clearing it would invalidate pointers held by libsignal-protocol-c.
        m_handshakeInProgress.clear();

        m_localDb = new wizz::client::LocalDatabase(dbPath);
        if (!m_localDb->init()) {
            qWarning() << "[Security] Failed to initialize SQLite database for" << username;
            return;
        }
 
        wizz::client::crypto::setupSignalCryptoProvider(&m_signalContext);
        wizz::client::crypto::setupSignalStoreContext(&m_storeContext, m_signalContext, m_localDb);
        wizz::client::crypto::initializeLocalKeys(m_signalContext, m_localDb);
        
        // --- UPLOAD KEYS TO SERVER ---
        std::vector<uint8_t> pubKey, privKey;
        uint32_t regId = 0;
        if (m_localDb->getIdentity(pubKey, privKey, regId)) {
            wizz::Packet uploadPkt(wizz::PacketType::UploadPreKeys);
            
            // Extract Identity Public Key (Binary Safe)
            uploadPkt.writeData(pubKey.data(), pubKey.size());

            // Extract Signed PreKey (from DB record)
            std::vector<uint8_t> spkBuf;
            if (m_localDb->loadSignedPreKey(1, spkBuf)) {
                session_signed_pre_key *spkRecord = nullptr;
                session_signed_pre_key_deserialize(&spkRecord, spkBuf.data(), spkBuf.size(), m_signalContext);
                
                ec_key_pair *pair = session_signed_pre_key_get_key_pair(spkRecord);
                ec_public_key *pub = ec_key_pair_get_public(pair);
                signal_buffer *pubSerialized = nullptr;
                ec_public_key_serialize(&pubSerialized, pub);
                uploadPkt.writeData(signal_buffer_data(pubSerialized), signal_buffer_len(pubSerialized));
                
                const uint8_t *sigData = session_signed_pre_key_get_signature(spkRecord);
                size_t sigLen = session_signed_pre_key_get_signature_len(spkRecord);
                uploadPkt.writeData(sigData, sigLen);
                
                uploadPkt.writeInt(regId);
                
                signal_buffer_free(pubSerialized);
                SIGNAL_UNREF(spkRecord);
            }

            // Extract One-Time Pre-Keys — collect first, then write actual count
            struct OtkEntry { int id; std::string pub; };
            std::vector<OtkEntry> otkEntries;
            for (int i = 1; i <= 100; ++i) {
                std::vector<uint8_t> pkBuf;
                if (m_localDb->loadPreKey(i, pkBuf)) {
                    session_pre_key *pkRecord = nullptr;
                    if (session_pre_key_deserialize(&pkRecord, pkBuf.data(), pkBuf.size(), m_signalContext) == 0 && pkRecord) {
                        ec_key_pair *pair = session_pre_key_get_key_pair(pkRecord);
                        ec_public_key *pub = ec_key_pair_get_public(pair);
                        signal_buffer *pubSerialized = nullptr;
                        ec_public_key_serialize(&pubSerialized, pub);
                        otkEntries.push_back({i, std::string(reinterpret_cast<const char*>(signal_buffer_data(pubSerialized)), signal_buffer_len(pubSerialized))});
                        signal_buffer_free(pubSerialized);
                        SIGNAL_UNREF(pkRecord);
                    }
                }
            }
            // Write the REAL count so the server reads exactly this many pairs
            uploadPkt.writeInt(static_cast<uint32_t>(otkEntries.size()));
            for (const auto& otk : otkEntries) {
                uploadPkt.writeInt(static_cast<uint32_t>(otk.id));
                uploadPkt.writeData(otk.pub.data(), otk.pub.size());
            }
            sendPacket(uploadPkt);
            std::cerr << "[Security] Dispatched E2EE public keys to server for " << username.toStdString() << std::endl;
        }

        m_e2eeInitialized = true;
        qDebug() << "[Security] E2EE isolation complete for" << username;
    } catch (const std::exception& e) {
        qWarning() << "[Security] Critical failure during E2EE isolation:" << e.what();
    }
}

NetworkManager::~NetworkManager() {
    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    if (m_storeContext) {
        signal_protocol_store_context_destroy(m_storeContext);
        m_storeContext = nullptr;
    }
    if (m_signalContext) {
        signal_context_destroy(m_signalContext);
        m_signalContext = nullptr;
    }
    if (m_localDb) {
        delete m_localDb;
        m_localDb = nullptr;
    }
    m_addressRegistry.clear();
    m_handshakeInProgress.clear();
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
    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    if (!m_e2eeInitialized || !m_localDb || !m_storeContext || !isConnected()) return;

    wizz::Packet pkt(wizz::PacketType::UploadPreKeys);

    // 1. Identity Key Public
    std::vector<uint8_t> pubKey, privKey;
    uint32_t regId = 0;
    if (m_localDb->getIdentity(pubKey, privKey, regId)) {
        pkt.writeData(pubKey.data(), pubKey.size());
    } else {
        pkt.writeData(nullptr, 0);
    }

    // 2. Signed PreKey Public & Signature — deserialize from libsignal protobuf record
    std::vector<uint8_t> signedPrekeyBuf;
    if (m_localDb->loadSignedPreKey(1, signedPrekeyBuf) && signedPrekeyBuf.size() > 0) {
        session_signed_pre_key *spkRecord = nullptr;
        int res = session_signed_pre_key_deserialize(&spkRecord,
                                                      signedPrekeyBuf.data(),
                                                      signedPrekeyBuf.size(),
                                                      m_signalContext);
        if (res == 0 && spkRecord) {
            signal_buffer *spkPubBuf = nullptr;
            ec_public_key_serialize(&spkPubBuf, ec_key_pair_get_public(session_signed_pre_key_get_key_pair(spkRecord)));
            
            pkt.writeData(signal_buffer_data(spkPubBuf), signal_buffer_len(spkPubBuf));
            pkt.writeData(session_signed_pre_key_get_signature(spkRecord), session_signed_pre_key_get_signature_len(spkRecord));

            signal_buffer_free(spkPubBuf);
            SIGNAL_UNREF(spkRecord);
        } else {
            std::cerr << "[Security] Failed to deserialize SignedPreKey for upload! Error: " << res << std::endl;
            pkt.writeData(nullptr, 0);
            pkt.writeData(nullptr, 0);
        }
    } else {
        pkt.writeData(nullptr, 0);
        pkt.writeData(nullptr, 0);
    }

    // 3. Registration ID
    pkt.writeInt(regId);

    // 4. PreKeys (OTKs) — deserialize from libsignal protobuf records
    pkt.writeInt(100);
    for (uint32_t i = 1; i <= 100; i++) {
        std::vector<uint8_t> prekeyBuf;
        if (m_localDb->loadPreKey(i, prekeyBuf) && prekeyBuf.size() > 0) {
            session_pre_key *pkRecord = nullptr;
            int res = session_pre_key_deserialize(&pkRecord, prekeyBuf.data(), prekeyBuf.size(), m_signalContext);
            if (res == 0 && pkRecord) {
                signal_buffer *pkPubBuf = nullptr;
                ec_public_key_serialize(&pkPubBuf, ec_key_pair_get_public(session_pre_key_get_key_pair(pkRecord)));
                pkt.writeInt(i);
                pkt.writeData(signal_buffer_data(pkPubBuf), signal_buffer_len(pkPubBuf));
                signal_buffer_free(pkPubBuf);
                SIGNAL_UNREF(pkRecord);
            }
        }
    }

    sendPacket(pkt);
    std::cerr << "[Security] Signal Identity Key & PreKey Bundle pushed to server!" << std::endl;
}

void NetworkManager::sendEncryptedMessage(const QString &target, const QString &text) {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "sendEncryptedMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, target), Q_ARG(QString, text));
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    
    if (!m_e2eeInitialized || !m_storeContext || !m_signalContext || !isConnected()) return;

    QString normalizedTarget = normalizeUsername(target);
    
    // --- SESSION GUARD ---
    // If no session exists, we must initiate a handshake first.
    std::string signalKey = normalizedTarget.toStdString() + ":1";
    if (!m_localDb->containsSession(signalKey)) {
        qDebug() << "[E2EE] No session exists for '" << normalizedTarget << "'. Initiating handshake...";
        if (!m_handshakeInProgress.contains(normalizedTarget)) {
            std::cerr << "[E2EE] No session exists for '" << normalizedTarget.toStdString() << "'. Initiating handshake..." << std::endl;
            m_handshakeInProgress.insert(normalizedTarget);
            
            wizz::Packet fetchPkt(wizz::PacketType::FetchPreKeyBundle);
            fetchPkt.writeString(normalizedTarget.toStdString());
            sendPacket(fetchPkt);
        }
        
        m_pendingMessages[normalizedTarget].append(text);
        return;
    }

    const signal_protocol_address* address = getSignalAddress(normalizedTarget);
    doSendEncryptedMessage(address, normalizedTarget, text);
}

void NetworkManager::doSendEncryptedMessage(const signal_protocol_address* address, const QString& target, const QString& text) {
    if (!m_e2eeInitialized || !m_storeContext || !m_signalContext || !address) return;
    
    session_cipher *cipher = nullptr;
    int res = session_cipher_create(&cipher, m_storeContext, address, m_signalContext);
    if (res < 0) {
        std::cerr << "[Security] Failed to create Session Cipher for " << target.toStdString() << " Error: " << res << std::endl;
        return;
    }

    ciphertext_message *encrypted = nullptr;
    QByteArray plainData = text.toUtf8();
    res = session_cipher_encrypt(cipher, reinterpret_cast<const uint8_t*>(plainData.constData()), plainData.size(), &encrypted);

    if (res == SG_ERR_NO_SESSION) {
        std::cerr << "[E2EE] No session with " << target.toStdString() << " (detected during encrypt) - fetching PreKey Bundle..." << std::endl;
        m_pendingMessages[target].append(text);

        wizz::Packet fetchPkt(wizz::PacketType::FetchPreKeyBundle);
        fetchPkt.writeString(target.toStdString());
        sendPacket(fetchPkt);
    } else if (res == 0) {
        signal_buffer *serialized = ciphertext_message_get_serialized(encrypted);
        std::vector<uint8_t> diagBytes(signal_buffer_data(serialized), signal_buffer_data(serialized) + std::min((size_t)signal_buffer_len(serialized), (size_t)8));
        std::stringstream ss;
        for(auto b : diagBytes) ss << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
        std::cerr << "[E2EE] Message to '" << target.toStdString() << "' encrypted. Len: " << signal_buffer_len(serialized)
                  << " Hex prefix: " << ss.str() << " Type: " << (int)(diagBytes[0] & 0x0F) << std::endl;

        wizz::Packet e2ePkt(wizz::PacketType::E2EMessage);
        e2ePkt.writeString(target.toStdString());
        e2ePkt.writeInt(static_cast<uint32_t>(signal_buffer_len(serialized)));
        e2ePkt.writeData(signal_buffer_data(serialized), signal_buffer_len(serialized));
        sendPacket(e2ePkt);

        SIGNAL_UNREF(encrypted);
    } else {
        std::cerr << "[E2EE] session_cipher_encrypt failed for '" << target.toStdString() << "'. Error code: " << res << std::endl;
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
    std::cout << "[NetworkManager] ContactList Entry: " << name.toStdString() << " Status: " << status << std::endl;
    contacts.append(std::make_tuple(name, status, statusMsg));
  }
  m_cachedContacts = contacts;
  emit contactListReceived(contacts);
}

void NetworkManager::handleContactStatusChangePacket(wizz::Packet &pkt) {
  int status = static_cast<int>(pkt.readInt());
  QString username = QString::fromStdString(pkt.readString());
  QString statusMsg = QString::fromStdString(pkt.readString());
  std::cout << "[NetworkManager] StatusChange Arrival: " << username.toStdString() << " -> " << status << std::endl;
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
    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    if (!m_storeContext || !m_signalContext) return;

    QString target = normalizeUsername(QString::fromStdString(pkt.readString()));
    
    // Binary safe reads for 33-byte keys and 64-byte signature
    std::vector<uint8_t> identVec = pkt.readBytes(33);
    std::vector<uint8_t> signedPreKeyVec = pkt.readBytes(33);
    std::vector<uint8_t> signatureVec = pkt.readBytes(64);
    
    uint32_t regId = pkt.readInt();
    uint32_t otkId = pkt.readInt();
    
    // OTK might be empty
    std::vector<uint8_t> otkVec;
    uint32_t otkDataLen = pkt.remainingSize() > 0 ? 33 : 0; // The server writes 33 bytes or 0
    if (otkDataLen > 0) {
        otkVec = pkt.readBytes(otkDataLen);
    }

    std::cerr << "[Handshake] Bundle for '" << target.toStdString() << "': regId=" << regId
              << " otkId=" << otkId
              << " identLen=" << identVec.size()
              << " signedKeyLen=" << signedPreKeyVec.size()
              << " sigLen=" << signatureVec.size()
              << " otkLen=" << otkVec.size() << std::endl;

    ec_public_key *identKey = nullptr;
    ec_public_key *signedPreKey = nullptr;
    ec_public_key *otk = nullptr;

    int decodeIdent = curve_decode_point(&identKey, identVec.data(), identVec.size(), m_signalContext);
    int decodeSigned = curve_decode_point(&signedPreKey, signedPreKeyVec.data(), signedPreKeyVec.size(), m_signalContext);
    std::cerr << "[Handshake] curve_decode_point: ident=" << decodeIdent << " signed=" << decodeSigned << std::endl;

    if (!otkVec.empty()) {
        int decodeOtk = curve_decode_point(&otk, otkVec.data(), otkVec.size(), m_signalContext);
        std::cerr << "[Handshake] curve_decode_point: otk=" << decodeOtk << std::endl;
    }


    if (!identKey || !signedPreKey) {
        std::cerr << "[Handshake] FATAL: Failed to decode public keys from bundle! Aborting session build." << std::endl;
        if (identKey) SIGNAL_UNREF(identKey);
        if (signedPreKey) SIGNAL_UNREF(signedPreKey);
        if (otk) SIGNAL_UNREF(otk);
        return;
    }

    session_pre_key_bundle *bundle = nullptr;
    int bundleRes = session_pre_key_bundle_create(&bundle, regId, 1, otkId, otk, 1, signedPreKey,
                                   signatureVec.data(), signatureVec.size(), identKey);
    std::cerr << "[Handshake] session_pre_key_bundle_create result: " << bundleRes << std::endl;

    const signal_protocol_address* address = getSignalAddress(target);

    session_builder *builder = nullptr;
    session_builder_create(&builder, m_storeContext, address, m_signalContext);
    int res = session_builder_process_pre_key_bundle(builder, bundle);
    std::cerr << "[Handshake] session_builder_process_pre_key_bundle result: " << res << std::endl;

    m_handshakeInProgress.remove(target);

    if (res == 0) {
        std::cerr << "[Handshake] Session with '" << target.toStdString() << "' established successfully! Sending " << m_pendingMessages[target].size() << " pending messages." << std::endl;
        
        for (const QString &msg : m_pendingMessages[target]) {
            doSendEncryptedMessage(address, target, msg);
        }
        m_pendingMessages.remove(target);
    } else {
        std::cerr << "[Handshake] FAILED to process PreKey bundle for '" << target.toStdString() << "'. Error code: " << res << std::endl;
        
        // AGGRESSIVE SELF-HEALING: If it's an identity mismatch (Error 1), purge EVERYTHING related to this participant.
        if (res == 1) {
            purgeParticipantState(target);
            // On the next message send attempt, it will fetch the bundle again starting 100% fresh.
        }
    }

    session_builder_free(builder);
    session_pre_key_bundle_destroy(reinterpret_cast<signal_type_base*>(bundle));
    SIGNAL_UNREF(identKey);
    SIGNAL_UNREF(signedPreKey);
    if (otk) SIGNAL_UNREF(otk);
}

void NetworkManager::handleE2EMessagePacket(wizz::Packet &pkt) {
    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    
    if (!m_e2eeInitialized || !m_storeContext || !m_signalContext) {
        return;
    }
    QString sender = QString::fromStdString(pkt.readString());
    uint32_t len = pkt.readInt();
    std::vector<uint8_t> data = pkt.readBytes(len);

    std::stringstream ss;
    for(size_t i=0; i < std::min((size_t)data.size(), (size_t)16); ++i) ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    std::cerr << "[E2EE] Received message from '" << sender.toStdString() << "' Len: " << data.size() 
              << " Hex prefix: " << ss.str() << std::endl;

    QString cleanedSender = normalizeUsername(sender);
    const signal_protocol_address* address = getSignalAddress(cleanedSender);

    std::optional<std::string> decryptedText = wizz::client::crypto::decryptMessage(
          cleanedSender.toStdString(), data, m_localDb, m_signalContext, m_storeContext);

    if (decryptedText.has_value()) {
        QString text = QString::fromStdString(decryptedText.value());
        std::cerr << "[E2EE] Message decrypted successfully. Relaying to UI..." << std::endl;
        emit messageReceived(sender, text);
    } else {
        std::cerr << "[E2EE] FATAL: Crypto Provider rejected payload for '" << cleanedSender.toStdString() << "'." << std::endl;
    }
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

void NetworkManager::purgeParticipantState(const QString &target) {
    std::lock_guard<std::recursive_mutex> lock(m_cryptoMutex);
    QString cleanedTarget = normalizeUsername(target);
    const signal_protocol_address* addr = getSignalAddress(cleanedTarget);
    std::string addr_str = std::string(addr->name) + ":" + std::to_string(addr->device_id);
    
    std::cerr << "[Handshake] PURGING all participant state for [" << addr_str << "]..." << std::endl;
    
    // 1. Wipe Session
    m_localDb->deleteSession(addr_str);
    
    // 2. Wipe Remote Identity
    m_localDb->deleteRemoteIdentity(addr_str);
    
    // 3. Clear transient handshake flags
    m_handshakeInProgress.remove(target);
    m_handshakeInProgress.remove(cleanedTarget);
}

QString NetworkManager::normalizeUsername(const QString &username) {
    return username.trimmed().toLower();
}
