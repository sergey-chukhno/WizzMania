# Feature Analysis: Games Integration (TileTwister & BrickBreaker)

## 1. Implementation Mechanics
WizzMania acts as a central hub (a launcher) for two natively integrated C++ graphical games: **TileTwister** (a Tetris clone built in SDL2) and **BrickBreaker** (built in SFML).

**Core Components:**
- **Process Spawning:** In `MainWindow::launchGame`, rather than calling a C++ function to start the game loop, we utilize `QProcess::startDetached()`.
- **Dynamic Path Resolution:** Because CMake outputs executables to different folders based on the OS (e.g., `build/games/TileTwister` vs `build/games/TileTwister/Release/TileTwister.exe`), the client dynamically constructs absolute file paths relative to `QCoreApplication::applicationDirPath()`.
- **Standalone Execution:** Once `QProcess` executes the game binary, the OS scheduler takes over. The game runs in its own memory space, opening its own Vulkan/OpenGL window, completely independent of the Qt graphical thread.

---

## 2. Advantages of the Architecture
- **Perfect Crash Isolation (Fault Tolerance):** C++ games are mathematically complex and highly prone to memory leaks or segmentation faults. If `TileTwister` attempts to access a null pointer and violently crashes, the OS kills the `TileTwister` child process. Crucially, the parent `wizz_client` process remains completely unaffected. The user's chat session is never interrupted by a game bug.
- **Framework Independence:** This architecture completely eliminated "Dependency Hell." Qt6 uses its own rendering pipeline, SDL2 uses its own, and SFML uses its own. By forcing them into separate executables, we avoided catastrophic CMake linker linker conflicts and OpenGL context collisions that occur when trying to embed SDL windows inside Qt widget containers natively.
- **Modular Deployment:** Developers can build, test, and iterate on `BrickBreaker` independently without ever needing to launch or compile the network chat client.

---

## 3. Drawbacks and Limitations
- **Zero Inter-Process Communication (IPC):** Because the games are isolated OS processes, WizzMania cannot natively talk to them. It is currently impossible to implement features like:
  - "WizzMania displays your live high score in your chat status."
  - "Click 'Invite to Game' in the chat to automatically connect TileTwister to your friend's TileTwister instance."
- **Distribution Complexity:** To ship this to a user, we cannot simply provide a single `WizzMania.exe`. We must package an entire folder hierarchy containing the client executable, the multiple game executables, and all the independent asset folders (`ttf` fonts, `.png` sprites, and `.wav` sounds) relative to their respective binaries.
- **Lack of Window Docking:** Because the games spawn OS-level windows, they cannot be "docked" cleanly inside the WizzMania interface. The user must manage multiple floating windows on their desktop.

---

## 4. Future Roadmap
To solve the IPC limitation, we would need to implement **Shared Memory (`QSharedMemory`)** or Local Sockets/Pipes. The game binaries could write their real-time state (e.g., "Score: 1000") to a shared RAM block, and a `QTimer` in the `wizz_client` would poll that memory block and broadcast it to the server.
