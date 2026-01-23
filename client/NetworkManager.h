#pragma once

#include "../common/Packet.h"
#include <QObject>
#include <QTcpSocket>
#include <memory>

class NetworkManager : public QObject {
  Q_OBJECT

public:
  static NetworkManager &instance();

  // Connection
  void connectToHost(const QString &host, quint16 port);
  void disconnectFromHost();
  bool isConnected() const;

  // Sending Data
  void sendPacket(const wizz::Packet &packet);

signals:
  // Status Signals
  void connected();
  void disconnected();
  void errorOccurred(QString errorMsg);

  // Data Signals (To be expanded)
  void packetReceived(const wizz::Packet &packet); // Raw packet
  void contactListReceived(const QList<QString> &contacts);

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

  QTcpSocket *m_socket;
  std::vector<uint8_t> m_buffer; // Receive buffer
};
