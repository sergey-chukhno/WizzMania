# WizzMania Strategy & Standards 📈🛡️

This document is the **authoritative source of truth** for the long-term vision, engineering philosophy, and feature roadmap of WizzMania. All architectural decisions must be traceable to a requirement defined here.

---

## 1. Engineering Standards (Modern C++17)

WizzMania adheres to strict Modern C++ standards to ensure memory safety, performance, and maintainability.

### 1.1 RAII & Resource Management
- **Rule**: Every resource (Heap, Socket, File, Thread) must be owned by an object.
- **Destructors**: Objects must automatically release resources in their destructors (e.g., `TcpServer` closing the server socket).
- **No Manual Management**: Raw `new`/`delete` and `malloc`/`free` are strictly forbidden. Use `std::unique_ptr` or `std::shared_ptr`.

### 1.2 Type-Safe Communication (Signals & Slots)
We utilize Qt's meta-object system for cross-thread communication, ensuring that the GUI thread is never blocked by network latency and that data moves safely between threads without crude mutex locks.

### 1.3 Exception Handling & Safety
- **Bounds Checking**: Every protocol read is wrapped in bounds checking to prevent buffer-overflow vulnerabilities.
- **Exceptions over Error-Codes**: We use standard C++ exceptions for logic errors, ensuring they cannot be ignored and that the stack unwinds safely (respecting RAII).

### 1.4 Code Quality Principles
- **Single Responsibility (SRP)**: Each class has one reason to change. UI classes do not contain business logic; network classes do not render widgets.
- **DRY (Don't Repeat Yourself)**: Shared logic (styles, packet builders, validators) is factored into reusable utilities.
- **KISS (Keep It Simple)**: Complexity is only introduced when it solves a real, present problem — not a hypothetical future one.
- **Clean Architecture**: Dependencies flow inward. UI depends on Services; Services depend on Domain Models; Domain Models depend on nothing.

---

## 2. Long-Term Innovation Roadmap: WizzMania Pro

WizzMania's vision is to evolve from a C++ Messenger into a **production-grade, privacy-first Super App** — combining the security of Signal, the breadth of Telegram, and the social ecosystem of WeChat, with architectural innovations that exceed all three.

---

### ✅ Pillar 4: The Shell (Premium UX) — *Phase Complete*

The foundational visual and navigational shell has been fully delivered.

**Delivered:**
- **Glassmorphism Design System**: Unified semi-transparent surfaces (`rgba(255,255,255,25)`), 20px rounded corners, specular 1px borders, and drop shadows across all components.
- **Hybrid Scroll Architecture**: Stable, clip-free Messenger layout with a fixed search header and a unified scrollable body containing the Contact List and Social Arcade.
- **Navigation Hub**: Multi-page `QStackedWidget` navigation (Messenger, Groups, Channels, Settings) controlled by the `NavigationHub` component.
- **Social Arcade Shell**: Collapsible, glassmorphic tile grid with 340px fixed expanded height and animated toggle.
- **Authentication Pro Shell**: Glassmorphic Login & Register cards with transparent vector iconography.
- **Component Consistency**: Synchronized button styles, hover effects, and cursor policies across all interactive elements.

**Remaining Shell Work:**
- Implement functional content for Groups, Channels, and Settings pages (currently placeholder stubs).
- Conduct a Clean Architecture audit of `MainWindow.cpp` to extract page controllers into dedicated classes.

---

### 🚧 Pillar 1: The Shield (Advanced Security & Privacy) — *In Progress*

**Delivered:**
- Signal Protocol integration (Double Ratchet) for 1:1 E2EE messaging.
- OpenSSL 3.0 bridge via `SignalProvider`.
- Zero-Knowledge relay on the server (blind forwarding of ciphertext).
- TLS 1.2 transport layer.

**Planned:**
- **Group E2EE (Sender Keys)**: Extend the Signal Protocol to group conversations using the Sender Key Distribution scheme. Each group member derives a shared chain key from the group's Sender Key, enabling O(1) encryption per message regardless of group size.
- **Secure VoIP (SRTP)**: All audio/video streams must be encrypted using SRTP (Secure Real-Time Transport Protocol) with DTLS-SRTP handshake for key exchange.
- **Sovereign Identity (Web3-Linked)**: Ed25519 identity keys double as Web3 wallet keys for native peer-to-peer identity verification.
- **Traffic Padding**: Dummy packet injection to mask communication metadata and prevent traffic pattern analysis.

---

### 🚧 Pillar 2: The Neural (Functional Local AI) — *Planned*

- **WizzNeural Node**: Native `llama.cpp` integration within an asynchronous Actor-model worker thread, isolated from the GUI thread.
- **Real-time Summarization**: Catch up on long group threads with a locally-generated summary.
- **Semantic Search**: Vector-indexed message history enabling natural language search (e.g., "find the photo Alex sent last week").
- **Smart Reply Suggestions**: Context-aware reply candidates generated locally, with no data leaving the device.

---

### 🚧 Pillar 3: The Ghost (Resilience & Scale) — *Planned*

**Delivered:**
- Asio-based asynchronous server with `shared_ptr`-managed sessions.
- Store-and-Forward offline message delivery.
- Binary protocol with 12-byte header and CRC32 validation.

**Planned:**
- **Group & Channel Orchestration**: Server-side `RoomManager` to handle group membership, broadcaster permissions, and message fan-out to N subscribers.
- **UDP Transport Layer**: Dedicated UDP pipeline for VoIP and real-time game state synchronization, using a custom reliability layer (selective ACKs) over raw UDP to avoid TCP head-of-line blocking.
- **TURN/STUN Relay**: Server-assisted NAT traversal for P2P VoIP when direct connection is not possible.
- **BLE Mesh (Off-Grid)**: Bluetooth Low Energy as a primary transport for internet-free peer discovery and message relaying.
- **Zero-Copy Protocol**: Refactoring the Asio/Packet layer for O(1) serialization with zero heap fragmentation using pre-allocated ring buffers.

---

### 🚧 Pillar 5: The Super App Ecosystem — *New*

WizzMania evolves from a Messenger into a platform. The Social Arcade is the entry point.

**Concept:**
The Social Arcade acts as a **Host Environment** for Mini-Apps. Each tile is a registered integration, not a hardcoded widget. A Mini-App can be a Game, a Music player, a shared Whiteboard, or a third-party integration.

**Planned:**
- **Mini-App Registry**: A manifest-based system where features register themselves (title, icon, launch command) — the Arcade renders from the registry, not hardcoded tiles.
- **Sandboxed Game Launcher**: Games run as isolated child processes communicating with the Shell via the existing Native Shared Memory IPC layer.
- **Social Arcade Categories**: Games, Music (collaborative playlist), Video (synchronized watch parties), and Community (polls, events, shared boards).
- **Live Activity Tiles**: Tiles show real-time activity (e.g., "3 friends playing TileTwister") using the presence infrastructure.

---

## 3. Project Constraints (MVP Phase)

While we innovate, the core project must remain:
1. **Portable**: Single "CMake Configure & Build" experience across WSL, Windows, and macOS.
2. **Scalable**: Capable of handling 1,000+ simultaneous SSL connections via the Asio event loop.
3. **Secure**: 100% test coverage on all cryptographic and authentication paths.
4. **Documented**: Every feature and architectural decision must be reflected in the `docs/` directory before it can be considered "done."
