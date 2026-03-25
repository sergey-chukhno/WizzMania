# Slide 3: Architecture Client-Serveur (Global Overview)

## 1. Feature Overview
This slide provides the "Big Picture" of WizzMania. it illustrates the flow of data from Independent Games -> Shared Memory -> Qt Client -> TLS Sockets -> Asio Server -> SQLite DB.

## 2. How it Works
The architecture follows a dual-path logic:
- **Vertical Path (Client-Server)**: Traditional async networking via TLS.
- **Horizontal Path (Games-Client)**: High-speed local IPC (Inter-Process Communication) using POSIX Shared Memory.

## 3. Why This Code (Rationale)
We chose the **Proactor Pattern** (via Boost.Asio). 
- **Why not Reactor?** Proactor (Asio's model) handles the completion of I/O operations asynchronously, whereas Reactor (like standard `select`/`poll`) only notifies you of readiness. Proactor is generally more efficient on modern OSs (using EPOLL/IOCP) and leads to cleaner code when using Lambda callbacks.

## 4. How the Code Implements This
In `server/main.cpp`:
- We initialize a single `asio::io_context`.
- Multiple threads can call `io_context.run()`, creating a **Thread Pool** that handles thousands of concurrent sessions.
- In `common/GameIPC.h`, the use of `#pragma pack(1)` ensures that a 32-bit game compiled with SFML and a 64-bit client compiled with Qt see the exact same binary layout in RAM.

## 5. Strategic Analysis

### Advantages
- **Decoupling**: The games (TileTwister/BrickBreaker) don't need to know the Server exists. They just write to RAM. This makes adding new games trivial.
- **Latency**: Using IPC for local scores is near-zero latency compared to a local TCP loopback.

### Drawbacks / Limits
- **Complexity**: Managing two distinct communication stacks (TLS and IPC) increases the surface area for bugs (e.g., race conditions in shared memory).

### Edge Cases
- **Zombie Segments**: If a game crashes without cleaning up its POSIX SHM segment, the client might read stale data.
- **Endianness**: While both sides are usually x86/ARM little-endian today, the `Packet` protocol explicitly uses **Network Byte Order** (Big-Endian) for safety, whereas SHM is native (High-performance).
