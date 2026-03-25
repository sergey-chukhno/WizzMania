# Slide 8: Jeux Indépendants & IPC (Shared Memory)

## 1. Feature Overview
This slide explains the magic of "Live Broadcast". How a standalone game (SFML/SDL2) sends its score to the WizzMania client without any direct networking.

## 2. How it Works
1. **Segment Creation**: The game (Player A) creates a POSIX Shared Memory segment in `/dev/shm`.
2. **Writing**: The game writes its state (score, game name) into this RAM segment at 60 FPS.
3. **Reading**: The WizzMania client maps the same segment and reads the data.
4. **Broadcast**: The client sends a `PacketType::GameStatus` to the server, which forwards it to all online friends.

## 3. Why This Code (Rationale)
- **POSIX `shm_open` / `mmap`**: This is the fastest way for two processes to talk. It is effectively "Zero-Copy" communication within the CPU cache/RAM.
- **`#pragma pack(1)`**: This is critical. Different compilers or architectures might add "Padding" bytes between fields in a struct. `pack(1)` ensures there is ZERO padding, so the layout is identical in the SFML game and the Qt client.

## 4. How the Code Implements This
### `GameIPCData` struct
The "Contract". It contains `isPlaying`, `currentScore`, and `gameName`. By making this a fixed-size struct, we avoid complex serialization.
### `NativeSharedMemory::lock()`
Uses `sem_wait` (POSIX Semaphores). This ensures that the client doesn't read the memory while the game is halfway through writing it (Atomic-like consistency).
### `GameStatusHandler::handle()` (Server)
A **Fan-out** logic. It iterates through the `onlineUsers` map and sends the score packet to everyone on the sender's friend list.

## 5. Strategic Analysis

### Advantages
- **Performance**: Zero syscalls for reading once mapped. No context switching to the kernel (like Pipes or Sockets).
- **Simplicity for Game Devs**: A game developer only needs a tiny header file (`GameIPC.h`) to integrate with WizzMania.

### Drawbacks / Limits
- **Local Only**: IPC only works if the game and the client are on the same machine.
- **Persistence**: If the client doesn't poll fast enough, it might miss a momentary score peak.

### Edge Cases
- **Race Conditions**: If a developer forgets to `unlock()` the semaphore, both processes will hang.
- **Naming Collisions**: We use "Wizz_IPC_" + username. If two users share a machine (WSL shared), we need to ensure the keys are truly unique.
