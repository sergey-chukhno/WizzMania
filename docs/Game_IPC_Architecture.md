# WizzMania Architecture Deep Dive: Inter-Process Communication (IPC)

As WizzMania evolved from a simple messaging client into a multimedia platform hosting standalone games, we encountered a fundamental architectural challenge: **Process Isolation**.

Our games—`TileTwister` (built on SDL2) and `BrickBreaker` (built on SFML)—are compiled as native, standalone executable binaries. They run in entirely separate operating system processes from the main Qt-based WizzMania messenger application. This isolation is fantastic for stability; if BrickBreaker crashes due to a physics bug, the main messenger stays perfectly intact. However, because they are isolated processes, **they share absolutely no memory**. 

To solve this and achieve our goal of broadcasting live game scores to the user's friends list, we needed a way for these separate processes to rapidly exchange data without the overhead of localhost network sockets. This is where **Inter-Process Communication (IPC)**, specifically Shared Memory, comes into play.

---

## 1. The Core Concept: POSIX Shared Memory

Shared memory is exactly what it sounds like: we ask the Operating System (macOS, in our case) to carve out a specific block of RAM and give both the game (the "Writer") and WizzMania (the "Reader") pointers to that exact same physical hardware location.

### Defining the Payload (`common/GameIPC.h`)
First, both ends need to agree on exactly *how* that abstract block of bytes is structured. We defined a fixed-layout C++ struct:

```cpp
namespace wizz {
    struct GameIPCData {
        bool isPlaying;               // Is a game currently actively running?
        int32_t currentScore;         // The live score of the running game
        char gameName[64];            // Null-terminated string (e.g., "TileTwister")
    };
}
```
*Note: We used primitive types and fixed-size char arrays (`char[64]`) rather than `std::string`. C++ `std::string` uses heap-allocated pointers under the hood. If we put a pointer in shared memory, the WizzMania client would try to dereference a memory address that only exists in the Game's private heap, leading to an immediate segmentation fault (crash). Fixed-size arrays guarantee all data lives inside the shared block.*

---

## 2. The Game Layer (The Writer)

Inside the games, we constructed the `NativeSharedMemory` class. This class uses raw, low-level POSIX system calls to interact with the OS.

### A. Memory Mapping (`shm_open` & `mmap`)
When TileTwister boots up, it asks the OS to open a shared memory file descriptor using `shm_open`. It specifies a highly unique, user-scoped key (e.g., `/Wizz_IPC_Mr_Krab`). It then uses `mmap` (Memory Map) to map that file descriptor into its own recognizable C++ pointer space. 

### B. Race Conditions and Mutexes (`sem_open`)
What happens if WizzMania tries to read the `GameIPCData` struct at the *exact millisecond* TileTwister is halfway through updating the score? WizzMania might read garbage, corrupted data. This is a **Race Condition**.

To prevent this, we paired our Shared Memory with a **Named Semaphore** (`sem_open`). A semaphore is an OS-level lock mechanism. 
When TitleTwister's game loop wants to update the score:
1. It calls `sharedMemory_->lock()`. If WizzMania is currently reading, the game thread pauses (blocks) until WizzMania is done.
2. It mutates the struct: `data->currentScore = newScore;`.
3. It calls `sharedMemory_->unlock()`, giving WizzMania permission to look at it again.

---

## 3. The Client Layer (The Reader)

Over in the WizzMania Qt Client (`MainWindow.cpp`), we possess the exact same username (`Mr_Krab`). The client has no idea *when* the user will score a point, so it utilizes a Polling architecture.

### Passive Polling (`QTimer` & `QSharedMemory`)
We initialized a `QSharedMemory` object with the same target key: `/Wizz_IPC_Mr_Krab`. 
Qt provides an incredibly useful tool called `QTimer`. We set a `QTimer` to tick every 2-3 seconds. On every tick, the client wakes up and checks the shared memory:
1. Client calls `m_sharedMemory.lock()`.
2. It casts the raw bytes back into a `GameIPCData*`.
3. It checks if `isPlaying` is true. If it is, and if the `currentScore` is different from the score it saw 3 seconds ago, it triggers a network event.
4. Client calls `m_sharedMemory.unlock()`.

This polling loop is extremely lightweight. It happens on the main GUI thread, but because it only runs every few seconds and purely reads a few bytes of RAM, it adds zero perceptible lag to the UI.

---

## 4. The Network Layer (The Broadcaster)

Once the `QTimer` detects a score change, WizzMania must transmit that information to the backend server so your friends can see it.

### Client-to-Server
WizzMania constructs a `GameStatus` packet containing the `gameName` and `currentScore`. It pushes this packet through the `NetworkManager` up to the Boost.Asio `TcpServer`.

### Server-to-Followers (`TcpServer.cpp`)
When the server receives this packet, it needs to perform relational routing:
1. It queries the `SQLite` Database: `"Who are the followers of Mr_Krab?"`
2. It iterates through that resulting list.
3. It checks its internal RAM dictionary (`m_onlineUsers`) to see which of those followers are currently connected to the server via an active TCP socket.
4. It rapidly fires off a corresponding `GameStatus` packet down the tubes directly to Bob, Patrick, and anyone else actively watching.

### The Initial Handshake Desync (Phase 4.2 Fix)
We discovered a fascinating edge case: What if Mr Krab is already playing a game, and *then* Bob logs in? Because Mr Krab has been repeatedly broadcasting his status to whoever was online at the time, Bob missed the memo and sees Mr Krab as offline.

We fixed this in `TcpServer::handleLogin()`. Now, the moment a new client connects, the Server behaves proactively:
1. It fetches Bob's friend list from the database.
2. It iterates through those friends, checks their *current* live status via `m_userStatuses`, and artificially builds/sends `ContactStatusChange` packets so Bob's UI accurately paints green dots and game icons before he even sees the main window.

---

### Architectural Summary
Through this multi-stage pipeline, we achieved a modular, crash-resilient ecosystem. The Games focus purely on rendering and writing to local RAM. The Client strictly monitors RAM and proxies to the Network. The Server acts as a pure relational router for connected sockets. This separation of concerns is a hallmark of senior-level, scalable application design!
