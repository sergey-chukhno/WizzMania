# WizzMania IPC & Multiplayer Game Integration Architecture

## Overview
WizzMania implements a multi-process architecture where the main Qt-based client (`wizz_client`) manages matchmaking, chat, and networking, while a suite of standalone SFML games (like `TicTacToe`, `TileTwister`, and `CyberpunkCannonShooter`) render the graphics and gameplay. To seamlessly connect these separate executables, WizzMania uses an Inter-Process Communication (IPC) layer powered by local Shared Memory.

This architectural split provides multiple benefits:
1. **Crash Isolation:** If a game crashes due to an SFML logic error or segmentation fault, the main WizzMania chat client remains unaffected.
2. **Framework Independence:** The UI relies on Qt and QML, while games rely on SFML/OpenGL. Keeping them in separate processes avoids framework initialization conflicts and nested event loops.
3. **Dedicated Performance:** Games can run their own dedicated game loop threads without blocking the main application UI thread.

## Inter-Process Communication (IPC) Mechanism
The IPC layer facilitates real-time communication between the `wizz_client` process and the launched game process. 

### The `wizz::GameIPCData` Structure
At the core of the IPC is the `GameIPCData` C++ structure (defined in `common/NativeSharedMemory.h`). This struct is a fixed-size memory block mapped into the RAM of both the Qt client and the SFML game. It contains:

- **State Flags:** E.g., `isPlaying`, `shouldClose`, `isPlayerTurn`.
- **Match Identifiers:** The `roomId` and `opponentName`.
- **Game-Specific Data:** Arrays describing the current game state, such as `board[9]` for TicTacToe.
- **Player Details:** For example, the `symbol` (X or O) representing the local player.

### Native Shared Memory Classes
To interact with the shared memory block, we use two classes tailored for their respective frameworks:

1. **`QSharedMemory` (Qt Client Side):**
   The WizzMania client uses Qt's `QSharedMemory`. It generates a unique memory region key based on the room and opponent's name (e.g., `WTT_<hash>`). When a game is negotiated via server WebSockets, the client prepares the memory block and writes the initial parameters.

2. **`wizz::NativeSharedMemory<T>` (SFML Game Side):**
   SFML games cannot use Qt classes easily (to avoid linking massive Qt binaries just for a small game). Instead, they use a custom, lightweight templated class `wizz::NativeSharedMemory<wizz::GameIPCData>`. 
   - On **macOS / Linux (WSL)**, this translates to pure POSIX functions (`shm_open`, `mmap`, `sem_open`) to map the memory region into the game's address space.
   - On **Windows**, it transparently falls back to Windows native memory mapping (`CreateFileMappingA`, `MapViewOfFile`).

## The Lifecycle of a Multiplayer Game

### 1. Matchmaking & Invitation
When Player A challenges Player B in the WizzMania UI, a `GameInvite` JSON packet is sent over the WebSocket to the WizzMania Server. The server routes it to Player B. When Player B hits "Accept", a `GameStart` packet tells both clients a game is ready.

### 2. Initialization & Launch
Inside `GameLauncher::launchTicTacToe(...)` (or equivalent):
- The `wizz_client` creates a QSharedMemory segment.
- It writes the opponent's name, the user's symbol (X or O), and sets `isPlaying = true`.
- If an avatar is provided, it exports the image to a temporary path (like `/tmp/ttt_avatar_...png`) so the SFML game can load it.
- Finally, it uses `QProcess` to spawn the SFML executable (e.g., `./bin/TicTacToe`), passing the room parameters via command-line arguments.

### 3. Gameplay Synchronization
Once the SFML game opens, it enters its `PlayingState`:
- **Reading:** It constantly polls the memory via `wizz::NativeSharedMemory` to see if `isPlayerTurn` has flipped to true, or if `board` changes occurred.
- **Writing:** When the player clicks a cell, the SFML game writes the move to the `board` array and sets a flag denoting a local move.
- **Networking:** The main WizzMania Qt client reads the shared memory. If it detects a local move, it packages it into a `GameMove` WebSocket packet and broadcasts it to the WizzMania server.
- **Opponent Move:** The server sends the move to the opponent's Qt client, which writes the move into *their* shared memory block. Their SFML game reads it, updates the screen, and the cycle continues.

### 4. Game Conclusion
When the game ends (Win, Lose, or Draw), the SFML game sets `shouldClose = true`. The `wizz_client` detects this, closes the `QSharedMemory` block, and broadcasts the completion status to the server, cleaning up the UI gracefully.
