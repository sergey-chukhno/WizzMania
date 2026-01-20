# Server Architecture & Concurrency Model

## 1. Overview
The Wizz Mania Server (`server/`) is designed as a **Single-Threaded Event Loop** (Reactor Pattern).
It uses non-blocking I/O multiplexing (`select()` or `poll()`) to handle multiple concurrent clients without the overhead of threads.

---

## 2. Design Choice: Single-Threaded vs. Multi-Threaded

### Option A: Thread-Per-Client (Rejected)
*   **Mechanism:** `accept()` a connection -> `std::thread(handle_client).detach()`.
*   **Pros:** Easy to write conceptually (linear code).
*   **Cons:**
    *   **Race Conditions:** If User A sends a message to User B, Thread A works on Socket B. If Thread B disconnects at the same time, we have a crash. Requires complex Locking (`std::mutex`).
    *   **Resource Heavy:** 1,000 clients = 1,000 threads. Heavy confusing context switching for the OS.

### Option B: Event Loop / IO Multiplexing (Selected)
*   **Mechanism:** One thread monitors ALL sockets. "Wake me up if ANY socket has data."
*   **Pros:**
    *   **No Locks:** Only one thing happens at a time. No race conditions.
    *   **Efficient:** Can handle thousands of idle connections with near-zero CPU usage.
    *   **Educational:** Demonstrates deep understanding of OS Networking primitives.

---

## 3. Real World Industry Examples

### WhatsApp (Erlang Model)
WhatsApp does not use C++ threads directly. It uses **Erlang Processes** (Green Threads).
*   **Architecture:** Massive scaling. One tiny "Actor" process per user.
*   **Why:** If one user crashes, it doesn't affect the million others. Erlang is built for Telecom reliability.

### Telegram / Modern C++ Servers (Redis, Node.js, Envoy)
High-performance servers typically use **Event Loops**, but scaled across CPU cores (Multi-Reactor).
*   **Redis / Node.js:** Strictly Single-Threaded. (Exactly like our approach).
*   **Envoy Proxy / Nginx:** A pool of worker threads (e.g., 8 threads for 8 CPUs). **Each worker** runs its own Single-Threaded Event Loop.

### Why Wizz Mania uses Single-Threaded
For a 2-week project, the Multi-Reactor (Thread Pool) adds synchronization complexity without meaningful performance gain on a laptop scale. Single-threaded is robust, simpler to debug, and fast enough for <10,000 users.

---

## 4. Implementation Plan
We will implement a wrapper class `TcpServer` covering:
*   `socket()`, `bind()`, `listen()` (Setup)
*   `accept()` (New connection)
*   `select()` (The Event Monitor that waits)

## 5. The "Socket Dance"
To establish a listening server, the OS requires a specific sequence of System Calls:

1.  **`socket()`**
    *   *Analogy:* Getting a telephone.
    *   *Action:* Creates a communication endpoint. Returns an ID (file descriptor).
2.  **`bind()`**
    *   *Analogy:* Assigning a phone number to that phone.
    *   *Action:* Assigns a specific IP (0.0.0.0) and Port (8080) to the socket.
    *   *Constraint:* Only one process can bind to a port at a time. If used, fails with `EADDRINUSE`.
3.  **`listen()`**
    *   *Analogy:* Plugging the phone into the wall and turning on the ringer.
    *   *Action:* Marks the socket as "Passive" (ready to accept incoming calls).
4.  **`accept()`**
    *   *Analogy:* Picking up the handset when it rings.
    *   *Action:* Blocks until a client connects, then returns a **new, separate socket** dedicated to that specific conversation.

