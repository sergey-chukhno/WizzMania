# WizzMania Architecture Q&A: Pillar 2 (Database Threading)

This document serves as the Mentorship Q&A for Pillar 2: Clean Architecture. It details the rationale, design, and implementation of the concurrent database operations we introduced to WizzMania.

## Q: Why did we need to move the database to a background thread?

Before our refactor, the WizzMania `TcpServer` ran as a single-threaded, synchronous application powered by the `select()` event loop. Whenever a network request arrived that asked for database access (like logging in, registering, or chatting), the `select()` loop had to pause and wait for the SQLite database query to finish natively on the disk. 

**The Problem:** I/O operations (like reading/writing to a database on disk) are exponentially slower than CPU operations (like parsing a network packet). If the database disk took 100ms to read a large `avatar` image blob, the server would literally freeze for 100ms. In a chat application meant for scale, if 10 users logged in simultaneously taking 100ms each, the 11th user would be completely unable to connect for a full second. 

**The Solution:** By moving the `DatabaseManager` to its own background thread, the network thread can quickly validate the packet, hand the "work order" off to the database thread, and immediately return to accepting new client connections.

## Q: The curriculum mentioned Qt `QThread`. Why did we use C++17 `std::thread` instead?

This was a major (and positive!) architectural pivot discovered during analysis. WizzMania consists of multiple modules. While the _client_ application heavily utilizes Qt (for its GUI components like `QMainWindow`, `QVBoxLayout`, `QPushButton`), the _server_ application had been built entirely using **Pure Modern C++17**.

**Benefits of pure C++ on the Server:**
Using Qt on a headless backend server introduces unnecessary dependency bloat. Qt is a massive framework meant primarily for Graphical User Interfaces. Keeping the server lightweight, with zero external dependencies (aside from standard C++ libraries and SQLite) means:
1.  **Faster Compilation:** The server builds in a fraction of the time.
2.  **Smaller Binary Size:** The server executable is tiny.
3.  **Maximum Portability:** The server can be deployed to minimal environments (like a bare-bones Linux Docker container or a Raspberry Pi) without needing hundreds of megabytes of Qt `.so` or `.dll` libraries installed. 

Therefore, we implemented the equivalent architecture—the **Actor Model**—using standard algorithms (`std::mutex`, `std::condition_variable`, and `std::thread`).

## Q: What is the "Actor Model" we implemented?

The **Actor Model** is a sophisticated architectural pattern for concurrent computation, originally conceptualized in 1973. In traditional Shared Memory concurrency, multiple threads attempt to read and write to the exact same variables simultaneously, requiring developers to defensively sprinkle `std::mutex` locks across their entire business logic. This approach is notoriously prone to deadlocks (where threads freeze waiting for each other) and race conditions (when a developer forgets a lock).

The Actor Model takes a radically different approach, championed by languages like Erlang and frameworks like Akka: **"Do not communicate by sharing memory; instead, share memory by communicating."**

In this architecture, an "Actor" is an isolated, independent entity that possesses three critical properties:
1.  **Strictly Private State:** The Actor encapsulates its own data (in our case, the SQLite database connection and the raw file descriptors). No outside thread is ever allowed to touch or mutually exclude this state.
2.  **An Asynchronous Inbox:** The Actor possesses a thread-safe mailbox (our `std::queue<std::function<void()>> m_tasks` guarded by a condition variable). This is the *only* place where a `std::mutex` is used, and it is heavily restricted to the microsecond it takes to push or pop a task.
3.  **Sequential Processing:** The Actor has its own dedicated worker thread. It loops infinitely, pulling one message out of the inbox at a time, and executing it to completion before touching the next.

**Why this is a Senior-Level Design Choice:**
By implementing the `DatabaseManager` as an Actor, we completely eliminated the need for complex locking inside our database query logic. The SQLite instance only ever experiences single-threaded, sequential access from the worker thread. The Network thread simply throws lambda functions (the "messages") into the queue via the `m_db.postTask()` construct and immediately returns to handling network traffic (fire-and-forget). This guarantees absolute data consistency without the performance penalty, cognitive load, and deadlock risks of pervasive shared-memory locking.

## Q: What are Race Conditions and Use-After-Free errors, and how did we prevent them?

**The Danger:**
When the Database Thread finishes its task (e.g., verifying a password), it needs to notify the client. But the network socket and `ClientSession` live on the Main Network Thread. 

If the database thread tries to directly call `session->sendPacket(...)`, what happens if the user suddenly disconnected a millisecond prior? 
1. The Main Thread deletes the `ClientSession` object from memory because the TCP connection died.
2. The Database Thread tries to access the memory address of that `ClientSession` to send the response.
3. **Result:** A Use-After-Free memory violation. The entire server crashes with a Segfault.

This is a classic **Race Condition**—the timing of events (network disconnect vs database finish) determines if the program survives.

**Our Mitigation Strategy:**
We designed a safe, two-way communication pipeline using task queues:
1.  The network thread asks the DB to do work: `m_db.postTask(...)`
2.  The DB does the work safely in the background.
3.  **The Critical Step:** The DB thread NEVER touches networking directly. Instead, it posts the _result_ back to the `TcpServer`'s response queue: `server->postResponse(...)`.
4.  The Main Network Thread, in its own loop, processes responses from the DB thread and sends packets down the socket.

Because the `TcpServer` processes the DB responses on the _exact same thread_ that handles disconnects, it can safely verify if the user is still connected before sending the packet:
```cpp
// Protected check executed entirely on the main thread:
ClientSession* session = server->getSession(socket);
if (!session) return; // User disconnected while DB was working! Abort cleanly.
```
This guarantees memory safety.

---
*Mentorship Note: This pattern—asynchronous dispatch queues with thread-safe callbacks—is the backbone of virtually all scalable concurrent systems (from Node.js event loops to Redis and NGINX). You have effectively built an enterprise-grade concurrency engine!*
