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

## 6. Data Processing: "The Pump"
To handle TCP fragmentation (partial packets), we implement a buffering strategy in `ClientSession`:

1.  **Accumulate:** Append all incoming bytes (`recv`) to a persistent `std::vector` buffer.
2.  **Pump Loop:**
    *   **Check 1:** Is buffer size >= Header Size (12 bytes)? If no, wait.
    *   **Peek:** Read the `BodyLength` from the header.
    *   **Check 2:** Is buffer size >= Header + BodyLength? If no, wait.
    *   **Extract:** Copy the full packet data, create a `Packet` object.
    *   **Consume:** Remove the processed bytes from the buffer (O(N) shift, but acceptable for <10KB packets).
    *   **Repeat:** Continue loop until buffer is empty or partial.

## 8. Messaging Architecture (The Hub Pattern)
To enable 1-to-1 messaging (User A -> User B), we treat the Server as the **Central Hub**.
*   **Problem:** `ClientSession A` is isolated. It has no pointer to `ClientSession B`.
*   **Naive Solution:** Give A a pointer to B.
    *   *Risk:* If B disconnects, A holds a **Dangling Pointer**. Crash.
*   **The Hub Solution:**
    1.  **Registry:** The Server maintains a Global Map: `std::unordered_map<std::string, ClientSession*> online_users`.
    2.  **Routing:**
        *   A sends `[Target="B", Msg="Hi"]` to Server.
        *   Server looks up "B" in `online_users`.
        *   If found: Server calls `B->send()`.
        *   If not found: Server stores message in DB (Offline).
*   **Source of Truth:** The Server is the *only* entity that knows who is online. Sessions are stateless regarding neighbors.

## 9. Session Registry Implementation (Callbacks)
To implement the "Hub Pattern" without tight coupling or circular dependencies, we use Modern C++ callbacks (`std::function`).
1.  **The Registry:** `TcpServer` holds `std::unordered_map<std::string, ClientSession*> m_onlineUsers`.
2.  **The Callback:** `ClientSession` holds a `std::function<void(ClientSession*)>` called `m_onLoginSuccess`.
3.  **The Flow:**
    *   `TcpServer` creates `ClientSession` and passes a lambda: `[this](ClientSession* s) { m_onlineUsers[s->getUsername()] = s; }`.
    *   When `ClientSession` authenticates (Login/Register), it invokes `m_onLoginSuccess(this)`.
    *   `TcpServer` receives the signal and updates the Registry.
This ensures `ClientSession` does not need to know about `TcpServer`, preserving the hierarchy.

## 10. Message Routing Logic
Messages (PacketType::DirectMessage) are routed using a similar callback pattern:
1.  User A sends `[Target="B", Msg="Hi"]`.
2.  `ClientSession` A reads the target and invokes `m_onMessage(this, "B", "Hi")`.
3.  `TcpServer` (the Router) looks up "B" in the `OnlineMap`.
4.  If B is found: Server constructs a packet `[Sender="A", Msg="Hi"]` and calls `B->sendPacket()`.
5.  If B is not found: The message must be stored for later delivery (See Section 11).

## 11. Offline Messaging (Future Work)
If the target user is not online, the message cannot be delivered immediately.
*   **Requirement:** Persistent storage of undelivered messages.
*   **Mechanism:**
    *   Server inserts message into a `messages` table in SQLite: `(sender, recipient, body, timestamp, delivered=0)`.
    *   When User B logs in, the Server queries this table for any pending messages.
    *   Server flushes pending messages to B and marks them as `delivered=1`.

