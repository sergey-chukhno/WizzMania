#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QPointer>
#include <QProcess>
#include <QPixmap>

#include "../common/NativeSharedMemory.h"
#include "../common/TicTacToeIPC.h"
#include "../common/GameIPC.h"

class GameBridge : public QObject {
  Q_OBJECT

public:
  explicit GameBridge(const QString& username, QObject* parent = nullptr);
  ~GameBridge();

  void startGameIPC();
  void stopGameIPC();

  void startTicTacToe(const QString& username, const QString& roomId, const QString& opponent, char symbol, const QPixmap& opponentAvatar);
  void stopTicTacToe();

  // Send an opponent's move to the running local game
  void receiveNetworkMove(uint8_t cellIndex);

signals:
  // Fired when the user's game state (Playing/Score) changes
  void localGameStatusChanged(bool isPlaying, const QString& gameName, uint32_t score);

  // Fired when the local game wants to send a move over the network to the opponent
  void localMoveMade(const QString& roomId, uint8_t cellIndex);
  
  // Fired when game ends or is aborted
  void ticTacToeFinished();

  // Fired specifically when the local player presses "Play Again"
  void rematchRequested(const QString& opponent);

private slots:
  void onPollGameIPC();
  void onPollTicTacToeIPC();

private:
  // User info
  QString m_username;

  // General Game Status IPC
  wizz::GameSharedMemory m_gameIPC;
  QTimer* m_gameIPCTimer = nullptr;
  bool m_lastIPCIsPlaying = false;
  uint32_t m_lastIPCScore = 0;
  QString m_lastIPCGameName;

  // TicTacToe specific IPC
  wizz::NativeSharedMemory<wizz::TicTacToeIPCData>* m_tttMemory = nullptr;
  QTimer* m_tttIPCTimer = nullptr;
  QString m_tttRoomId;
  QString m_tttOpponent;
  char m_tttSymbol;
  bool m_tttBridgeActive = false;
  bool m_tttGameOver = false;
  QPointer<QProcess> m_tttProcess;
};
