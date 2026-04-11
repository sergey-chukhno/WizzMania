# Pillar 4 Mentorship: Design & Scalability Analysis (Archived)

## 1. TCP Server & Event Loop Efficiency (Historical)
[Context: This analysis preceded the migration to Standalone Asio.]

**Current State (Historical):**
The server used a monolithic event loop powered by the POSIX `select()` API.
- **O(N) Traversal**: select() requires looping over all sockets twice per iteration.
- **FD_SETSIZE Limit**: Hardcoded to 1024 concurrent connections by the OS.

**Design Evolution:**
The server was eventually refactored to use **Reactor Patterns** via asynchronous frameworks, solving the O(N) bottleneck and allowing system-level event notification ($O(K)$).

[... See documentation/01_Architecture/Server.md for the current Asio-based implementation ...]

---

## 2. Heap Allocations & Memory Pooling
Analysis of memory fragmentation and the proposal for pre-allocated buffer pools for high-frequency packet processing.

---

## Mentor Q&A (Historical)
- **Q: Multiple threads for networking?** 
  - **A**: Lock-free concurrency and Reactor-per-Core models are the keys to global scale.
- **Q: Why was select() flawed?**
  - **A**: It told the server that *something* happened, but forced the server to search through every user to find out *what*. Modern epoll just hands you the active sockets.
