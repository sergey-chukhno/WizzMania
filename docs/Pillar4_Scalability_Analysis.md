# Pillar 4 Mentorship: Design & Scalability Analysis

## 1. TCP Server Connection Handling & Event Loop Efficiency

Currently, `wizz_server` uses a monolithic event loop powered by the POSIX `select()` API.
```cpp
// Inside TcpServer::run()
FD_ZERO(&readfds);
FD_SET(m_serverSocket, &readfds);
// ... adding all client sockets ...
int activity = select(max_sd + 1, &readfds, nullptr, nullptr, &timeout);
```
**Efficiency Breakdown:**
- **Single-threaded Event Loop:** The server utilizes a single thread for all network I/O. While this avoids race conditions on shared state like `m_onlineUsers`, it caps the I/O throughput to a single CPU core. 
- **O(N) Traversal:** For every event loop iteration, the server loops over *all* connected clients to add them to `fd_set`, and then loops over them *again* to check `FD_ISSET`. This creates a lot of wasted CPU cycles when most sockets are effectively idle.

## 2. The Limits of `select()` vs `poll()` vs `epoll` / `kqueue`

**`select()`:**
- **Mechanism:** Passes fixed-size bitmasks (`fd_set`) of file descriptors to the kernel.
- **Limits:** Hardcoded limit (usually `FD_SETSIZE = 1024`). The server categorically cannot handle more than 1024 concurrent users without a custom compilation of the kernel or C runtime.
- **Complexity:** `O(N)` where N is the highest file descriptor number. 

**`poll()`:**
- **Mechanism:** Passes an array of `pollfd` structs.
- **Limits:** No strict 1024 limit, allowing more connections.
- **Complexity:** Still `O(N)` because the array must be scanned entirely on every wake-up to figure out which sockets have data.

**`epoll` (Linux) / `kqueue` (macOS/BSD):**
- **Mechanism:** Event-based notification. You register a socket once, and the kernel tells you exactly which sockets are active when you call `epoll_wait()` or `kevent()`.
- **Limits:** Bound only by system memory and open file limits (hundreds of thousands of concurrent connections).
- **Complexity:** `O(K)` where K is the number of *active* events, ignoring the idle majority.
**Recommendation for Scale:** Transitioning `wizz_server` to an asynchronous framework like `Boost.Asio`, `libuv`, or raw `epoll`/`kqueue` is mandatory to grow beyond prototyping phases.

## 3. Heap Allocations & Memory Pooling

**Current Flow Analysis:**
Whenever a packet is received, the current `Packet` structure is likely instantiated on the stack, but internal vectors and `QByteArray`s are dynamically allocated on the heap. During heavy chat storms, the server acts as an aggressive allocator and deallocator, leading to:
- **Heap fragmentation.**
- **CPU time wasted on standard library allocator synchronization (locks inside `malloc`/`new`).**

**The Solution: Memory Pooling**
To scale, we can pre-allocate a "Pool" of fixed-size buffers (e.g., thousands of 4KB chunks).
1. When a client socket reads data, it borrows a chunk from the `BufferPool`.
2. When the packet is processed and routed, the chunk is returned to the pool.
3. Memory is never freed back to the OS until server shutdown; pointer bumping replaces costly syscalls.

---

## Mentor Q&A: Scalable Architectures

**Q: If we switch to multiple threads for networking, won't we need mutexes everywhere?**
**A:** If multiple threads read and write to the same `m_onlineUsers` map, yes. But a scalable architecture avoids locks. A common pattern is the **Reactor pattern** (one thread per core handling `epoll`), which push validated messages to a lock-free queue, consumed by dedicated **Worker Threads**.

**Q: Do we need a custom Memory Pool for everything?**
**A:** No, premature optimization is the root of all evil. For `wizz_server`, you'd profile first using tools like `Valgrind` or macOS `Instruments`. If memory allocation overhead exceeds 10-15% of your CPU time during load-tests, you implement a Memory Pool specifically for the `Packet` buffers, as those are the highest-frequency allocations.

**Q: How does the Database Thread fit into a massively concurrent architecture?**
**A:** Exactly as we restructured it in Pillar 2: via an asynchronous Actor Model. In massively concurrent systems, I/O bound tasks (like querying SQLite) must never block the event loop. The network threads drop tasks into the DB queue and move on, processing responses via callbacks when the DB thread completes its work. This ensures maximum responsiveness.

**Q: Are we using the Reactor Pattern for our TCP server? How does it differ from modern versions?**
**A:** Yes! Our `TcpServer` implements a custom, single-threaded Reactor pattern. 
- **Initiation Dispatcher:** The `select()` API acts as the dispatcher. Using a bitmask (`fd_set`), it queries the kernel to see which sockets have data ready.
- **Event Handlers:** Those are the `std::function` lambdas attached to each `ClientSession`. Once the dispatcher confirms data is ready, it triggers the handler (e.g., `handleLogin`) via `onDataReceived()`.

However, our Reactor's core mechanism—`select()`—has a fatal flaw: it is $O(N)$. `select()` tells the server that *some* sockets are ready, but not *which* ones. The server must spin through an expensive `for` loop over every connected client to check `FD_ISSET()`. If 10,000 users are mostly idle, the CPU wastes cycles checking empty sockets.

**Q: How does `epoll` or `kqueue` solve this flaw?**
**A:** Modern servers use Event-Triggered notifications ($O(K)$ complexity). When you use `epoll` (Linux) or `kqueue` (macOS), the kernel wakes the server up and provides a pre-filtered array of *only the specific sockets* that actually sent data. A burst of 5 active chatters among 10,000 connected peers takes exactly 5 iterations to process, eliminating massive CPU waste.

**Q: What would it take to refactor to a "Reactor-per-Core" model?**
**A:** Currently, our Reactor runs on a single network I/O thread, utilizing only 1 CPU core regardless of the server's hardware. To scale globally:
1. **Multi-Reactor Topology:** We would adopt an industry-standard asynchronous framework like *Boost.Asio*. We'd query the hardware for core count (e.g., 8 cores) and spawn 8 independent Reactor threads.
2. **Asynchronous Reading:** Replace blocking `recv()` loops with `async_read()`. The OS distributes incoming active sockets (abstracted cleanly via `epoll` under the hood) across the 8 threads.
3. **Lock-Free Concurrency:** **CRITICAL:** Multiplexing 8 TCP threads means `m_onlineUsers` is subject to chaotic race conditions if someone logs in while someone logs out. We would have to restructure our entire state management using advanced `std::shared_mutex` (Read-Write Locks) or lock-free Concurrent Hash Maps to prevent segmentation faults.
