# WizzMania — Architecture Refactoring Sprint

> **Sprint goal**: Eliminate God Objects, enforce SOLID principles, and introduce industry-standard design patterns to make the codebase modular, testable, and ready for future innovation phases (E2EE, Rate Limiting, Web3).

---

## Table of Contents

1. [Context & Motivation](#1-context--motivation)
2. [Before vs After — High-Level Architecture](#2-before-vs-after)
3. [Pattern 1 — Command Pattern (Packet Router)](#3-pattern-1--command-pattern-packet-router)
4. [Pattern 2 — Service Layer / Facade (SessionManager & GameRoomManager)](#4-pattern-2--service-layer--facade)
5. [Pattern 3 — Adapter / Bridge (GameBridge)](#5-pattern-3--adapter--bridge-gamebrige)
6. [Pattern 4 — Observer (Qt Signals & Slots)](#6-pattern-4--observer-qt-signals--slots)
7. [Pattern 5 — Singleton (NetworkManager)](#7-pattern-5--singleton-networkmanager)
8. [Pattern 6 — Actor Model (DatabaseManager)](#8-pattern-6--actor-model-databasemanager)
9. [Applied SOLID Principles](#9-applied-solid-principles)
10. [Performance Improvement: In-Session Contact Cache](#10-performance-improvement-in-session-contact-cache)
11. [Summary of Changed Files](#11-summary-of-changed-files)

---

## 1. Context & Motivation

Before the refactoring sprint, the codebase exhibited three major anti-patterns:

| Anti-Pattern | Location | Symptom |
|---|---|---|
| **God Object** | `TcpServer.cpp` | Handled login, routing, game rooms, avatars, status — all in one class |
| **God Object** | `MainWindow.cpp` | Mixed UI rendering with POSIX IPC, `QProcess` management, network calls |
| **Monolithic Parser** | `ClientSession.cpp` | A single giant `switch(packet.type())` block with hundreds of lines |

These violations of the **Single Responsibility Principle (SRP)** made the code hard to extend:  adding a new packet type required modifying both the session and the server, creating merge conflicts and regression risks.

---

## 2. Before vs After

```
BEFORE                               AFTER
────────────────────────             ────────────────────────
TcpServer                            TcpServer
  └── handleLogin()        ──►         └── registers handlers once
  └── handleMessage()                PacketRouter
  └── handleGameStatus()               └── dispatches to...
  └── m_onlineUsers (map)           ├── LoginHandler
  └── m_gameRooms (map)             ├── MessageHandler
  └── getFollowers() + SQL          ├── NudgeHandler
                                    ├── GameStatusHandler ...
                                    SessionManager
                                      └── online users, statuses
                                    GameRoomManager
                                      └── rooms, scores

MainWindow                           MainWindow
  └── QProcess (game)     ──►         └── connects signals only
  └── shm_open() / mmap()           GameBridge
  └── onPollTicTacToeIPC()            └── QProcess management
  └── NativeSharedMemory*             └── IPC polling timers
                                        └── emits typed signals
```

---

## 3. Pattern 1 — Command Pattern (Packet Router)

**Problem**: `ClientSession::onDataReceived` was a giant `switch` statement with inline handler logic. Every new packet type required opening the core session file.

**Solution**: `IPacketHandler` interface + `PacketRouter` dispatcher.

```cpp
// server/handlers/IPacketHandler.h
class IPacketHandler {
public:
    virtual void handle(ClientSession* session, wizz::Packet& packet) = 0;
    virtual ~IPacketHandler() = default;
};

// Concrete command — one file per responsibility
class LoginHandler : public IPacketHandler {
public:
    void handle(ClientSession* session, wizz::Packet& packet) override;
};

// Registration at startup — TcpServer constructor
m_packetRouter.registerHandler(PacketType::Login,        make_unique<LoginHandler>());
m_packetRouter.registerHandler(PacketType::GameInvite,   make_unique<GameInviteHandler>());
m_packetRouter.registerHandler(PacketType::GameStatus,   make_unique<GameStatusHandler>());
// ... 14 handlers total, each in its own file
```

**Files**: `handlers/IPacketHandler.h`, `handlers/AuthHandlers.cpp`, `handlers/SocialHandlers.cpp`, `handlers/GameHandlers.cpp`, `handlers/PacketRouter.cpp`

**OCP compliance**: Adding a new packet type now means creating a new file and one `registerHandler()` call — zero changes to existing code.

---

## 4. Pattern 2 — Service Layer / Facade

**Problem**: `TcpServer` directly held the `unordered_map<string, ClientSession*>` for online users, the `unordered_map<string, GameRoom>` for game state, and wrote SQL queries inline. This violated SRP and made handlers impossible to unit-test in isolation.

**Solution**: Two domain services extracted as dedicated classes.

### SessionManager
```cpp
// server/SessionManager.h
class SessionManager {
public:
    void setUserOnline(const string& username, ClientSession* s, const string& status);
    void setUserOffline(const string& username);
    ClientSession* getSessionByUsername(const string& username) const;
    int getStatus(const string& username) const;
    vector<string> getAllOnlineUsernames() const;
    // ...
private:
    unordered_map<string, ClientSession*> m_onlineUsers;
    unordered_map<string, int>            m_userStatuses;
    unordered_map<string, string>         m_customStatuses;
};
```

### GameRoomManager
```cpp
// server/GameRoomManager.h
class GameRoomManager {
public:
    string createRoom(const string& playerA, const string& playerB);
    ClientSession* getOpponent(const string& roomId, const string& requestingPlayer) const;
    void updateGameStatus(const string& username, const string& gameName, uint32_t score);
    void clearGameStatus(const string& username);
    // ...
private:
    unordered_map<string, GameRoom>       m_gameRooms;
    unordered_map<string, GameStatus>     m_gameStatuses;
};
```

**Benefit**: `TcpServer` is now just a network transport layer. Handlers call `server->getSessionManager()` and `server->getGameRoomManager()` — clean single-purpose interfaces (ISP compliance).

---

## 5. Pattern 3 — Adapter / Bridge (GameBridge)

**Problem**: `MainWindow` directly called `shm_open()`, `mmap()`, `sem_open()`, started `QProcess`, and managed a raw `NativeSharedMemory<TicTacToeIPCData>*` pointer. UI code and OS-level IPC were completely entangled.

**Solution**: `GameBridge` — a `QObject` adapter encapsulating all game process and IPC logic.

```cpp
// client/GameBridge.h
class GameBridge : public QObject {
    Q_OBJECT
public:
    void startTicTacToe(const QString& username, const QString& roomId,
                        const QString& opponent, char symbol,
                        const QPixmap& opponentAvatar);
    void receiveNetworkMove(uint8_t cellIndex);
    void stopTicTacToe();

signals:
    void localGameStatusChanged(bool isPlaying, const QString& gameName, uint32_t score);
    void localMoveMade(const QString& roomId, uint8_t cellIndex);
    void ticTacToeFinished();
    void rematchRequested(const QString& opponent); // "Play Again" → new invite

private:
    QPointer<QProcess>                          m_tttProcess;
    NativeSharedMemory<TicTacToeIPCData>*       m_tttMemory = nullptr;
    QTimer*                                     m_tttIPCTimer = nullptr;
};
```

**Key launch order fix**: `startTicTacToe` now launches the `QProcess` *first*, waits up to 4 seconds (20 × 200ms retries) for the game to create its POSIX shared memory segment, then attaches the IPC bridge — matching the actual lifecycle of the SFML/SDL process.

**Benefit**: `MainWindow` only `connect()`s signals. It never touches IPC or processes directly. Swapping POSIX shared memory for a pipe or socket later requires changes only in `GameBridge`.

---

## 6. Pattern 4 — Observer (Qt Signals & Slots)

Qt's built-in publish/subscribe mechanism is the backbone of client-side decoupling. Every cross-component event is routed through typed signals:

| Signal | Emitter | Connected Slot |
|---|---|---|
| `contactListReceived` | `NetworkManager` | `MainWindow::setContacts()` |
| `nudgeReceived` | `NetworkManager` | `MainWindow` → `ChatWindow::shake()` |
| `gameStartReceived` | `NetworkManager` | `MainWindow` → `m_gameBridge->startTicTacToe()` |
| `localMoveMade` | `GameBridge` | `MainWindow::onLocalMoveMade()` → `NetworkManager::sendGameMove()` |
| `rematchRequested` | `GameBridge` | `MainWindow` → `NetworkManager::sendGameInvite()` |
| `avatarUpdated` | `AvatarManager` | `MainWindow::updateContactAvatar()` |

No component holds a raw pointer to another — they are connected through signal/slot edges only.

---

## 7. Pattern 5 — Singleton (NetworkManager)

`NetworkManager` uses a thread-safe Meyer's Singleton to guarantee a single TLS socket exists per process. It lives on a dedicated `QThread` and all cross-thread calls are automatically marshalled via `QMetaObject::invokeMethod` with `Qt::QueuedConnection`.

```cpp
static NetworkManager& instance() {
    static NetworkManager inst; // Thread-safe in C++11+
    return inst;
}
```

---

## 8. Pattern 6 — Actor Model (DatabaseManager)

SQLite is single-writer by nature. `DatabaseManager` implements the Actor pattern: a dedicated worker thread owns the SQLite connection exclusively. All callers post lambdas to a `std::queue` protected by a `std::mutex` + `std::condition_variable`. Results are returned to the server's IO thread via `server->postResponse()`.

```
Caller Thread          DB Worker Thread
─────────────          ────────────────
postTask([]{           workerLoop():
  auto users =           while(true):
  db.getFriends(...)       task = queue.pop()
  postResponse([]{         task()     // runs SQL
    broadcast(...)
  });
})
```

This prevents DB I/O from blocking Boost.Asio's event loop, ensuring network throughput is not gated on disk latency.

---

## 9. Applied SOLID Principles

| Principle | How it's applied |
|---|---|
| **S** — Single Responsibility | Each handler class handles exactly one packet type. `TcpServer` handles only TCP/TLS acceptance. `GameBridge` handles only IPC. |
| **O** — Open/Closed | Adding a new packet type requires only a new `IPacketHandler` subclass and one `registerHandler()` call. No existing files are modified. |
| **L** — Liskov Substitution | All handlers are interchangeable implementations of `IPacketHandler`. The router calls `handle()` polymorphically without knowing the concrete type. |
| **I** — Interface Segregation | `SessionManager` and `GameRoomManager` expose narrow, purpose-specific APIs. Handlers only call what they need. |
| **D** — Dependency Inversion | Handlers receive `ClientSession*` and call `session->getServer()` to access services — programming to the server's public interface, not its implementation details. |

---

## 10. Performance Improvement: In-Session Contact Cache

**Problem**: `GameStatusHandler` was called every 100ms (IPC poll interval). Each call dispatched an async DB task to `SELECT` followers and friends — causing a queue of DB queries that arrived out of order and dropped broadcasts intermittently.

**Fix**: At login, `AuthHandlers::LoginHandler` stores the union of `followers ∪ friends` directly in the `ClientSession` object:

```cpp
// In AuthHandlers.cpp — after login completes
std::set<std::string> contactSet;
for (const auto& f : followers) contactSet.insert(f);
for (const auto& f : friends)   contactSet.insert(f);
s->setContacts(std::move(contactSet));
```

`GameStatusHandler` now broadcasts synchronously using this in-memory set:

```cpp
// In GameHandlers.cpp — zero DB round-trips
for (const auto& contactName : session->getContacts()) {
    ClientSession* target = server->getSessionManager().getSessionByUsername(contactName);
    if (target) target->sendPacket(pkt);
}
```

**Result**: Game status broadcasts are now synchronous, deterministic, and never dropped.

---

## 11. Summary of Changed Files

| File | Change |
|---|---|
| `server/TcpServer.h/.cpp` | Removed all handler logic; registers 14 handlers in constructor |
| `server/ClientSession.h/.cpp` | Removed `switch` block; added `m_contacts` cache; routes to `PacketRouter` |
| `server/SessionManager.h/.cpp` | **NEW** — owns online user map, statuses, custom statuses |
| `server/GameRoomManager.h/.cpp` | **NEW** — owns game rooms and game status state |
| `server/handlers/IPacketHandler.h` | **NEW** — command interface |
| `server/handlers/PacketRouter.h/.cpp` | **NEW** — dispatcher map |
| `server/handlers/AuthHandlers.h/.cpp` | **NEW** — `LoginHandler`, `RegisterHandler`; populates contacts cache |
| `server/handlers/SocialHandlers.h/.cpp` | **NEW** — `MessageHandler`, `NudgeHandler`, `StatusChangeHandler`, etc. |
| `server/handlers/GameHandlers.h/.cpp` | **NEW** — `GameStatusHandler` (synchronous), `GameInviteHandler`, `GameMoveHandler` |
| `client/GameBridge.h/.cpp` | **NEW** — IPC adapter; correct launch order; `rematchRequested` signal |
| `client/MainWindow.cpp` | Removed all IPC code; connects `GameBridge` signals; connects `rematchRequested` |
| `client/ChatWindow.cpp` | Added `Qt::WindowStaysOnTopHint` to prevent z-order click interception |
