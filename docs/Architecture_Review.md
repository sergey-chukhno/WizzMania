# WizzMania Architectural Review & Refactoring Proposal

This document provides a senior-level architectural analysis of the WizzMania codebase, evaluating its compliance with **SOLID**, **DRY**, and **KISS** principles. It identifies existing design patterns and proposes critical refactorings to ensure the platform is agile, maintainable, and scalable for future Innovation Roadmap phases (Web3, Mesh, AI).

## 1. Current State Assessment

### Strengths (What works well)
- **Asynchronous Foundation**: The server utilizes Boost.Asio (Reactor/Proactor pattern) effectively for non-blocking I/O.
- **Actor-ish Model (Database)**: Utilizing a dedicated worker thread and queue for SQLite operations prevents database locks from stalling the main networking thread.
- **Observer Pattern (Client)**: Extensive use of Qt's Signals and Slots keeps UI components reasonably decoupled from core network events.
- **Singleton**: `NetworkManager` properly centralizes socket state.

### Critical Anti-Patterns & Violations

#### A. The "God Object" (Violates Single Responsibility Principle - SRP)
* **`TcpServer`**: Currently acts as a monolithic God Object. It handles TCP connection acceptance, SSL handshakes, parsing internal state, managing `m_gameRooms`, `m_onlineUsers`, checking passwords, routing chat messages, and doing direct IPC logic. It knows too much about the *business rules* of the app, rather than just being a network transport layer.
* **`MainWindow`**: In the client, `MainWindow` manages the GUI layout but also directly handles `QProcess` for games and maps POSIX shared memory (`m_tttMemory`). It mixes Presentation Logic with OS-level Inter-Process Communication.

#### B. Tight Coupling (Violates Dependency Inversion Principle)
* `ClientSession` calls functions directly back onto `TcpServer` (e.g., `m_server.handleMessage(...)`). This creates a circular dependency where the Session knows the specific implementation details of the Server.

#### C. Missing Encapsulation (Violates DRY & KISS)
* **Packet Parsing**: Packet decoding (reading fields, lengths, etc.) is scattered across large switch statements in both client (`NetworkManager.cpp`) and server (`ClientSession.cpp`). If the protocol changes, multiple files break.
* **IPC Logic**: Shared memory management is raw and leaky inside `MainWindow`. It's not simple (KISS violation) for a UI window to manage memory mmap offsets.

---

## 2. Proposed Architectural Refactorings

To make the code modular and ready for Green C++ (Zero-copy), Web3, and Mesh networking, we must implement the following design patterns:

### Refactoring 1: Service Layer & Facade Pattern (Server-side)
* **Why**: Break down `TcpServer` into distinct managers.
* **How**: 
   - Introduce `SessionManager` (handles connection lifetimes).
   - Introduce `GameRoomManager` (handles TicTacToe/Arcade state).
   - Introduce `PresenceManager` (handles online statuses and broadcast trees).
* **Result**: `TcpServer` only accepts sockets and hands them to the `SessionManager`. Code becomes modular and highly testable.

### Refactoring 2: Command Pattern (Packet Handling)
* **Why**: Replace the giant `switch` statements for incoming packets.
* **How**: Create an `IPacketHandler` interface. Implement isolated command classes like `LoginCommand`, `SendMessageCommand`, `GameMoveCommand`. Use a `PacketRouter` (Strategy) to dispatch packets to their respective handlers.
* **Result**: Adding a new feature (like Web3 Crypto Payment Packet) will only require creating a new file, not modifying core monolith files (Open-Closed Principle).

### Refactoring 3: Repository Pattern (Database Access)
* **Why**: Clean up raw SQL strings scattered inside the DatabaseManager.
* **How**: Create distinct classes like `UserRepository`, `MessageRepository` which sit behind the `DatabaseManager` facade.
* **Result**: Easy to swap out SQLite for PostgreSQL later if scaling demands it.

### Refactoring 4: Adapter/Bridge Pattern for IPC (Client-side)
* **Why**: Remove low-level POSIX/Shared Memory code from `MainWindow`.
* **How**: Create a `GameBridge` class that encapsulates `QProcess` and memory mapping. It exposes simple signals like `gameMoveReceived(x, y)` to the `MainWindow`.
* **Result**: UI logic stays isolated. Replacing native shared memory with websockets or different IPC mechanisms in the future won't require touching the UI.

---

## 3. Implementation Plan (Next Steps)

Before adding new features, I propose a short "Refactoring Sprint" focusing on:
1. **Extracting Managers**: Pulling Game, User, and Session state out of `TcpServer`.
2. **Command Routing**: Refactoring `ClientSession`'s packet parsing into isolated Command handlers.
3. **Client IPC Bridge**: Extracting the Shared Memory logic out of `MainWindow`.

Doing this now will dramatically accelerate the implementation of E2EE, Rate Limiting, and Web3 features, as the code will be "pluggable" rather than intertwined.
