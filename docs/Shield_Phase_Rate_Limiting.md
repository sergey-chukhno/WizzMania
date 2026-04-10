# 🛡️ Shield Phase: Rate Limiting Architecture

This document describes the design and implementation of the **Rate Limiting (Shield)** layer in the WizzMania server, designed to protect the codebase against Denial of Service (DoS) attacks, brute-force attempts, and logic-loop bugs in clients.

## 🏛️ Core Design Principles

Our rate limiting is based on the **Token Bucket** algorithm, providing a balance between allowing short "bursts" of legitimate activity while enforcing a strict steady-state limit.

### 1. Dual-Layer Defense Strategy
The protection is implemented at two distinct levels of the networking stack:

| Layer | Protected Resource | Implementation | Mitigation |
|-------|-------------------|----------------|------------|
| **Connection Level** | IP Address (L3/L4) | [TcpServer.cpp](file:///Users/sergeychukhno/Desktop/C:C++/WizzMania/server/TcpServer.cpp) | Drop socket before TLS handshake |
| **Command Level** | Authenticated Session (L7) | [ClientSession.cpp](file:///Users/sergeychukhno/Desktop/C:C++/WizzMania/server/ClientSession.cpp) | Drop logical packet before processing |

---

## 🧮 Algorithm: Token Bucket
Implemented in [RateLimiter.h](file:///Users/sergeychukhno/Desktop/C:C++/WizzMania/server/RateLimiter.h).

Each bucket is defined by:
- **Capacity (Burst)**: The maximum number of tokens a bucket can hold (e.g., 5 connections allowed at once).
- **Refill Rate**: How many tokens are added back per second (e.g., 1 connection allowed per second).

### Thread-Safety
The `RateLimiter` uses a `std::mutex` to protect the bucket state, allowing for concurrent consumption from multiple threads (e.g., when multiple sessions from the same IP connect simultaneously).

---

## 🛠️ Implementation Details

### A. Connection Throttling (IP-based)
Triggered in the `TcpServer::doAccept` loop.
- **Goal**: Prevent a single external script from overwhelming the server with thousands of TCP/TLS connections.
- **Config**: 
  - `CONN_BURST = 5.0`
  - `CONN_RATE = 1.0` (1 new connection per second)
- **Logic**: If an IP exceeds the limit, the server immediately calls `socket.close()` **before** performing the expensive TLS handshake.

### B. Command Throttling (Session-based)
Triggered in the `ClientSession::processPacket` logic.
- **Goal**: Prevent a logged-in user (or a buggy client loop) from spamming Nudges, Messages, or Game Invites.
- **Config**: 
  - `Burst = 10.0`
  - `Rate = 3.0` (3 commands per second)
- **Logic**: If the session exceeds the limit, the packet is ignored, and a `[Shield]` warning is logged to the server console.

---

## 🧪 Verification & Testing

The Shield Phase is backed by a dedicated test suite verifying both the math and the integration.

### Unit Tests
Located in [test_rate_limiter_unit.cpp](file:///Users/sergeychukhno/Desktop/C:C++/WizzMania/tests/server/test_rate_limiter_unit.cpp).
- `BurstCapacity`: Verifies that consumption is capped at the initial burst size.
- `RefillLogic`: Verifies that tokens are accurately refilled over time.
- `ThreadSafety`: Verifies correct behavior under high concurrent load (100 parallel requests).

### Integration Tests
Located in [test_shield_integration.cpp](file:///Users/sergeychukhno/Desktop/C:C++/WizzMania/tests/server/test_shield_integration.cpp).
- Verifies that `ClientSession` correctly drops the 11th packet in a rapid sequence.

---

## ⚠️ Admin Monitoring
Monitor the server logs for the following tags:
- `[Shield] Dropping connection from X (Rate limit exceeded)`
- `[Shield] Dropping packet from session X (User Y) - Rate limit exceeded`

> [!TIP]
> If you notice legitimate users being throttled during high-activity gaming sessions, adjust the `Burst` and `Rate` constants in `TcpServer.h` and `ClientSession.cpp` to accommodate higher throughput.
