# 🛡️ Sentinel Phase: Observability & Terminal Dashboard

## 1. Overview
The **Sentinel Phase** transforms the WizzMania server from a silent background process into a fully observable, interactive administrative tool. It introduces a real-time Command Dashboard (TUI) and a high-performance metrics registry, enabling administrators to monitor traffic, track security events, and execute commands globally without halting server operations.

## 2. Core Architecture

### 2.1 Multithreaded Execution Model
To support a real-time UI without blocking network I/O, the server architecture was refactored:
- **Main Thread (UI/Admin):** Dedicated entirely to rendering the FTXUI dashboard, processing keyboard inputs, and executing administrative commands.
- **Networking Thread (I/O):** A dedicated background thread (`std::thread m_networkThread`) exclusively runs the Boost.Asio `io_context` event loop.
- **Concurrency Safety:** `Server::postResponse()` is used to safely transition commands from the UI thread directly into the networking event loop.

### 2.2 Lock-Free Metrics Registry (`MetricsManager`)
Traditional `std::mutex` locking introduces massive performance bottlenecks when tracking thousands of packets per second. To solve this, the `MetricsManager` was built using `std::atomic` counters.
- **Lock-Free Operations:** Utilizes `std::atomic<uint64_t>` to allow `m_metrics[MetricType].fetch_add(1, std::memory_order_relaxed)`.
- **Zero-Overhead Tracking:** Metrics can be updated instantly from any thread without risking deadlocks or slowing down socket reads.
- **Tracked Metrics:** Active Sessions, Total Connections, Packets In/Out, Traffic (Bytes) In/Out, Shield Drops (Rate Limiter), and Failed Logins.

## 3. The Sentinel Dashboard (FTXUI)
The presentation layer leverages the modern **FTXUI** library to create a dynamic, reactive terminal graphic interface. 

### Features:
- **Real-Time FPS Target:** The UI operates on a non-blocking background refresh loop configured to update at approximately 5 frames per second.
- **Component Layout:**
  - **Header Ribbon:** Displays Server Uptime (calculated from `std::chrono::steady_clock`) and overall Operational Status.
  - **Networking Stats:** A dynamic grid (`vbox` + `hbox` constraints) rendering raw metrics like byte-counts and connections.
  - **Security Shields:** A highlighted panel tracking hostile interactions, hooking directly into the server's Rate Limiter.
  - **Command Box & History:** A scrollable log of server events paired with an active input field for typing commands.

## 4. Administrative Command Processor
To interact with the server dynamically, the `CommandProcessor` interprets text input from the Sentinel Dashboard and executes actions.

### Supported Commands:
| Command | Parameter(s) | Action |
|---------|-------------|--------|
| `/broadcast` | `<message body>` | Safely dispatches a `DirectMessage` packet with the sender "SYSTEM" to all currently connected clients. |
| `/kick` | `<session id>` | Force-terminates a specific client session (currently stubbed for extension). |
| `/stats` | (None) | Acknowledges active tracking in the UI. |
| `/clear` | (None) | Purges the TUI console history. |
| `/shutdown` | (None) | Gracefully aborts networking threads, saves states, and halts the server. |

### Broadcasting Flow
1. User types `/broadcast Hello` in the UI thread.
2. `CommandProcessor` captures the string and invokes `TcpServer::broadcastMessage()`.
3. `TcpServer` wraps the logic in a lambda and posts it to the Boost.Asio I/O thread.
4. The I/O thread constructs the `PacketType::DirectMessage` and iterates over `getAllOnlineSessions()`, dispatching the packet to active clients.

## 5. Integration Hooks
The Observability layer is deeply integrated into the core components:
- **`TcpServer.cpp`:** Hooks into connection acceptance to increment `ConnectionsTotal` and `ActiveSessions`.
- **`ClientSession.cpp`:** Intercepts `onRead` and `doWrite` tasks to increment Byte scales and Packet counts precisely as data crosses the socket boundary.
- **`RateLimiter.h`:** Automatically triggers a `ShieldDrop` metric increment the moment a client exceeds the permitted packet-rate threshold. 
