# WizzMania Strategy & Standards 📈🛡️

This document outlines our engineering philosophy and the long-term vision to transform WizzMania into a premium, secure life-platform.

---

## 1. Engineering Standards (Modern C++17)
WizzMania adheres to strict Modern C++ standards to ensure memory safety, performance, and maintainability.

### 1.1 RAII & Resource Management
- **Rule**: Every resource (Heap, Socket, File) must be owned by an object.
- **Destructors**: Objects must automatically release resources in their destructors (e.g., `TcpServer` closing the server socket).
- **No Manual Management**: `new`/`delete` and `malloc`/`free` are strictly forbidden. Use `std::unique_ptr` or `std::shared_ptr`.

### 1.2 Type-Safe Communication (Signals & Slots)
We utilize Qt's meta-object system for cross-thread communication, ensuring that the GUI thread is never blocked by network latency and that data moves safely between threads without crude mutex locks.

### 1.3 Exception Handling & Safety
- **Bounds Checking**: Every protocol read is wrapped in bounds checking to prevent buffer-overflow vulnerabilities.
- **Exceptions over Error-Codes**: We use standard C++ exceptions for logic errors, ensuring they cannot be ignored and that the stack unwinds safely (respecting RAII).

---

## 2. Long-Term Innovation Roadmap

### Phase 1: Security Hardening (Current)
- **Zero-Knowledge Relay**: Fully implemented E2EE system.
- **Sentinel Observability**: Real-time monitoring and rate limiting.

### Phase 2: Social & Arcades (The "Hub" Phase)
- **ARCADE-HUB**: Centralized matchmaking for TicTacToe, Chess, and more.
- **LivePresence**: Rich status synchronization (e.g., what game or music is playing).

### Phase 3: Intelligence & Privacy (The "Brain" Phase)
- **Local Client AI**: Integrating `llama.cpp` for local, private conversation summarization and smart replies.
- **ShadowChats**: Implementation of disappearing messages with zero disk persistence.
- **Mesh Resilience**: Peer-to-Peer connectivity via BLE/Wi-Fi Direct for off-grid messaging.

### Phase 4: Extreme Sovereignty (The "Ghost" Phase)
- **Multi-Device Sync**: Federated Key Chain to securely synchronize E2EE sessions across multiple devices while maintaining Zero-Knowledge.
- **Sovereign Identity**: Utilizing cryptographic keys (Ed25519) instead of phone numbers for identity.
- **Traffic Padding**: Masking communication patterns with dummy packets to prevent traffic analysis.

---

## 3. Project Constraints (MVP Phase)
While we innovate, the core project must remain:
1.  **Portable**: Single "CMake Configure & Build" experience across WSL, Windows, and macOS.
2.  **Scalable**: Capable of handling 1,000+ simultaneous SSL connections via the Asio event loop.
3.  **Secure**: 100% test coverage on all cryptographic and authentication paths.
