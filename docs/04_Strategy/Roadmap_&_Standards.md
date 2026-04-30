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

## 2. Long-Term Innovation Roadmap: WizzMania Pro

### Pillar 1: The Shield (Advanced Security & Privacy)
- **Double Ratchet Integration**: Transitioning from static pairwise pre-keys to a dynamic Double Ratchet system for per-message Perfect Forward Secrecy.
- **Sovereign Identity (Web3-Linked)**: Utilizing Ed25519 cryptographic keys as the primary user identity. The chat identity key doubles as a **Web3 Wallet Key**, enabling native, peer-to-peer tipping.
- **Traffic Padding**: Masking communication metadata with dummy packet injection to prevent traffic pattern analysis.

### Pillar 2: The Neural (Functional Local AI)
- **WizzNeural Node**: Native `llama.cpp` integration within an asynchronous Actor-model worker.
- **Unified Insight Engine**: Local processing for both **Real-time Summarization** (catching up on threads) and **Semantic Search** (Vector-indexed history).

### Pillar 3: The Ghost (Resilience & Scale)
- **BLE Mesh FIRST**: Implementing Bluetooth Low Energy (BLE) as the primary off-grid transport layer for internet-free peer discovery and relaying.
- **Zero-Copy Protocol**: Refactoring the Asio/Packet layer for absolute O(1) serialization performance and zero heap fragmentation.

### Pillar 4: The Shell (Premium UX)
- **Futuristic Glassmorphism Overhaul**: A professional, translucent UI theme with micro-animations and custom high-DPI SVG iconography.

---

## 3. Project Constraints (MVP Phase)
While we innovate, the core project must remain:
1.  **Portable**: Single "CMake Configure & Build" experience across WSL, Windows, and macOS.
2.  **Scalable**: Capable of handling 1,000+ simultaneous SSL connections via the Asio event loop.
3.  **Secure**: 100% test coverage on all cryptographic and authentication paths.
