#pragma once

#include "../common/Packet.h"
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QSslError>
#include <QSslSocket>
#include <atomic>
#include <functional>
#include <memory>

class NetworkManager : public QObject {
  Q_OBJECT

public:
  static NetworkManager &instance();

  bool isConnected() const;
  QList<QPair<QString, int>> getContacts() const { return m_cachedContacts; }

  void initSocket();

public slots:
  // Connection
  void connectToHost(const QString &host, quint16 port);
  void disconnectFromHost();

  // Sending Data
  void sendPacket(const wizz::Packet &packet);
  void sendVoiceMessage(const QString &target, uint16_t duration,
                        const std::vector<uint8_t> &data);
  void sendTypingPacket(const QString &target, bool isTyping);
  void sendUpdateAvatar(const QByteArray &data);
  void requestAvatar(const QString &username);
  void sendStatusChange(int status, const QString &statusMessage = "");
  void sendGameStatus(const QString &gameName, uint32_t score);

signals:
  // Status Signals
  void connected();
  void disconnected();
  void errorOccurred(QString errorMsg);

  // Data Signals (To be expanded)
  void packetReceived(const wizz::Packet &packet); // Raw packet
  void contactListReceived(const QList<QPair<QString, int>> &contacts);
  void contactStatusChanged(const QString &username, int status);
  void messageReceived(const QString &sender, const QString &text);
  void nudgeReceived(const QString &sender);
  void voiceMessageReceived(const QString &sender, uint16_t duration,
                            const std::vector<uint8_t> &data);
  void userTyping(const QString &sender, bool isTyping);
  void avatarReceived(const QString &username, const QByteArray &data);
  void gameStatusChanged(const QString &username, const QString &gameName,
                         uint32_t score);

private slots:
  void onSocketConnected();
  void onSocketDisconnected();
  void onSocketError(QAbstractSocket::SocketError socketError);
  void onReadyRead();

private:
  explicit NetworkManager(QObject *parent = nullptr);
  ~NetworkManager();

  // Delete copy constructor and assignment operator
  NetworkManager(const NetworkManager &) = delete;
  NetworkManager &operator=(const NetworkManager &) = delete;

  QSslSocket *m_socket = nullptr;
  std::vector<uint8_t> m_buffer; // Receive buffer
  std::atomic<bool> m_isConnected{false};
  QList<QPair<QString, int>> m_cachedContacts;

  // Packet Handlers
  void registerHandlers();
  void handleContactListPacket(wizz::Packet &pkt);
  void handleContactStatusChangePacket(wizz::Packet &pkt);
  void handleErrorPacket(wizz::Packet &pkt);
  void handleDirectMessagePacket(wizz::Packet &pkt);
  void handleNudgePacket(wizz::Packet &pkt);
  void handleVoiceMessagePacket(wizz::Packet &pkt);
  void handleTypingIndicatorPacket(wizz::Packet &pkt);
  void handleAvatarDataPacket(wizz::Packet &pkt);
  void handleGameStatusPacket(wizz::Packet &pkt);

  QHash<wizz::PacketType, std::function<void(wizz::Packet &)>> m_packetHandlers;
};
