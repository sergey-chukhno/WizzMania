#include "GameBridge.h"
#include <QDebug>
#include <QThread>

GameBridge::GameBridge(const QString& username, QObject* parent) 
  : QObject(parent), m_username(username), m_gameIPC(wizz::makeIPCKey(username.toStdString())) {
}

GameBridge::~GameBridge() {
  stopGameIPC();
  stopTicTacToe();
}

void GameBridge::startGameIPC() {
  m_gameIPCTimer = new QTimer(this);
  connect(m_gameIPCTimer, &QTimer::timeout, this, &GameBridge::onPollGameIPC);
  m_gameIPCTimer->start(100);
}

void GameBridge::stopGameIPC() {
  if (m_gameIPCTimer) {
    m_gameIPCTimer->stop();
    m_gameIPCTimer->deleteLater();
    m_gameIPCTimer = nullptr;
  }
}

#include "GameLauncher.h"

void GameBridge::startTicTacToe(const QString& username, const QString& roomId, const QString& opponent, char symbol, const QPixmap& opponentAvatar) {
  stopTicTacToe();

  m_username = username;
  m_tttRoomId = roomId;
  m_tttOpponent = opponent;
  m_tttSymbol = symbol;
  m_tttBridgeActive = true;
  m_tttGameOver = false;

  // Step 1: Launch the game process FIRST so it can create the shared memory segment
  qDebug() << "[TicTacToe IPC] Launching TicTacToe process for room" << roomId;
  m_tttProcess = GameLauncher::launchTicTacToe(m_username, m_tttRoomId, m_tttSymbol, m_tttOpponent, opponentAvatar);
  if (!m_tttProcess) {
    qDebug() << "[TicTacToe IPC] Failed to launch TicTacToe process.";
    m_tttBridgeActive = false;
    return;
  }
  connect(m_tttProcess.data(), &QProcess::finished, this, [this]() {
    stopTicTacToe();
    emit ticTacToeFinished();
  });

  // Step 2: Now wait for the game to create its POSIX shared memory
  const std::string ipcKey = wizz::makeTicTacToeIPCKey(
      roomId.toStdString() + "_" + m_username.toStdString());

  m_tttMemory = new wizz::NativeSharedMemory<wizz::TicTacToeIPCData>(ipcKey);

  bool attached = false;
  for (int retry = 0; retry < 20 && !attached; ++retry) { // up to 4 seconds
    attached = m_tttMemory->openAndMap();
    if (!attached)
      QThread::msleep(200);
  }
  if (!attached) {
    qDebug() << "[TicTacToe IPC] Could not open POSIX shared memory for room"
             << roomId << "after" << "4s - IPC bridge disabled.";
    delete m_tttMemory;
    m_tttMemory = nullptr;
    // Game process is still running, just no IPC bridge
    m_tttBridgeActive = false;
    return;
  }

  qDebug() << "[TicTacToe IPC] Bridge attached for room" << roomId;

  m_tttIPCTimer = new QTimer(this);
  connect(m_tttIPCTimer, &QTimer::timeout, this, &GameBridge::onPollTicTacToeIPC);
  m_tttIPCTimer->start(50);
}

void GameBridge::stopTicTacToe() {
  if (m_tttIPCTimer) {
    m_tttIPCTimer->stop();
    m_tttIPCTimer->deleteLater();
    m_tttIPCTimer = nullptr;
  }
  if (m_tttMemory) {
    m_tttMemory->close();
    delete m_tttMemory;
    m_tttMemory = nullptr;
  }
  m_tttBridgeActive = false;
  m_tttGameOver = false;
  m_tttRoomId.clear();
  m_tttOpponent.clear();
  qDebug() << "[TicTacToe IPC] Bridge stopped.";
}

void GameBridge::receiveNetworkMove(uint8_t cellIndex) {
  if (m_tttMemory && m_tttBridgeActive && !m_tttGameOver) {
    m_tttMemory->lock();
    auto *data = m_tttMemory->data();
    if (data) {
      data->inboundCellIndex = cellIndex;
      data->hasInboundMove = true;
    }
    m_tttMemory->unlock();
  }
}

void GameBridge::onPollTicTacToeIPC() {
  if (!m_tttMemory || !m_tttBridgeActive) return;

  m_tttMemory->lock();
  auto *data = m_tttMemory->data();
  if (!data) {
    m_tttMemory->unlock();
    return;
  }

  if (!m_tttGameOver) {
    QString tttStatus = "TicTacToe vs " + m_tttOpponent;
    if (tttStatus != m_lastIPCGameName) {
      m_lastIPCGameName = tttStatus;
      emit localGameStatusChanged(true, tttStatus, 0);
    }

    if (data->hasOutboundMove) {
      int cellIndex = data->outboundCellIndex;
      data->hasOutboundMove = false;
      m_tttMemory->unlock();
      emit localMoveMade(m_tttRoomId, static_cast<uint8_t>(cellIndex));
      return;
    }

    if (data->gameOver) {
      m_tttGameOver = true;
      m_tttMemory->unlock();
      emit localGameStatusChanged(false, "", 0);
      emit ticTacToeFinished();
      return;
    }
    
    m_tttMemory->unlock();
    return;
  }

  if (data->rematchRequested || data->quitRequested) {
    bool isRematch = data->rematchRequested;
    QString opponent = m_tttOpponent; // capture before stopTicTacToe clears it
    m_tttMemory->unlock();
    stopTicTacToe();
    emit ticTacToeFinished();
    if (isRematch) {
      emit rematchRequested(opponent); // tell MainWindow to send a new invite
    }
    return;
  }

  m_tttMemory->unlock();
}

void GameBridge::onPollGameIPC() {
  bool opened = m_gameIPC.data() != nullptr;
  if (!opened) {
    opened = m_gameIPC.openAndMap();
    if (!opened) {
      if (m_lastIPCIsPlaying) {
        m_lastIPCIsPlaying = false;
        emit localGameStatusChanged(false, "", 0);
      }
      return;
    }
  }

  m_gameIPC.lock();
  const wizz::GameIPCData *data = m_gameIPC.data();
  if (data) {
    bool isPlaying = data->isPlaying;
    uint32_t currentScore = data->currentScore;
    QString gameName = QString::fromUtf8(data->gameName);

    if (isPlaying != m_lastIPCIsPlaying || currentScore != m_lastIPCScore ||
        gameName != m_lastIPCGameName) {
      m_lastIPCIsPlaying = isPlaying;
      m_lastIPCScore = currentScore;
      m_lastIPCGameName = gameName;

      emit localGameStatusChanged(isPlaying, gameName, currentScore);
    }
  }
  m_gameIPC.unlock();
}
