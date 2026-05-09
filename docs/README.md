# WizzMania Documentation Hub 🛡️🏗️

Welcome to the central technical documentation for WizzMania. This is the **single source of truth** for all architectural decisions, feature specifications, and engineering standards. All code changes must be traceable to a document in this hub.

**Last Updated**: 2026-05-09 (Super App specification — Groups, VoIP, Social Arcade, CodeRabbit CI gate)

---

## 🗺️ Documentation Sitemap

### [01. Architecture](./01_Architecture/Server.md)
The blueprint of our system — how it's built and how it works today.
- [Server & Protocol](./01_Architecture/Server.md): Standalone Asio, binary framing, modular DAOs, and thread isolation.
- [Client & Shell](./01_Architecture/Client.md): Threading model, E2EE layer, Pro Shell architecture, Glassmorphism design system, and known technical debt.
- [Security & E2EE](./01_Architecture/Security.md): TLS 1.2, Signal Protocol (Double Ratchet), and Shield protection.
- [IPC & Games Integration](./01_Architecture/IPC_&_Games.md): Native shared-memory synchronization and game-core isolation.

---

### [02. Features](./02_Features/Messaging.md)
Precise specifications for every user-facing capability — current and planned.

**Delivered:**
- [Reactive Messaging](./02_Features/Messaging.md): Typing indicators, presence sync, and the "Nudge."
- [Multimedia Exchange](./02_Features/Multimedia.md): Voice messaging (PCM/zlib/Opus) and Avatar management.

**Specified — Pending Implementation:**
- [Group Chats & Channels](./02_Features/Groups_and_Channels.md): E2EE groups (Sender Keys), role systems, channel broadcasting, and required protocol extensions.
- [Audio & Video Calls](./02_Features/Audio_Video_Calls.md): P2P VoIP (Opus/H.264), DTLS-SRTP encryption, ICE/STUN/TURN NAT traversal, and group call architecture.
- [Social Arcade & Super App](./02_Features/Social_Arcade_SuperApp.md): Mini-App SDK, manifest registry, Games/Music/Video/Community integrations, and live activity tiles.

---

### [03. Operations](./03_Operations/Environment_Setup.md)
Guides for developers and administrators.
- [Environment Setup](./03_Operations/Environment_Setup.md): Build instructions for WSL, Windows, and macOS.
- [Testing & Verification](./03_Operations/Testing_&_Verification.md): Guide to the test verification suite and security coverage.

---

### [04. Strategy](./04_Strategy/Roadmap_&_Standards.md)
The authoritative vision and engineering rules governing the project.
- [Innovation Roadmap & Standards](./04_Strategy/Roadmap_&_Standards.md): All five strategic Pillars (Shield, Neural, Ghost, Shell, Super App), engineering standards (RAII, SOLID, Clean Architecture), and project constraints.

---

## 🚀 Getting Started
To build the full project including all games and the test suite:
```bash
cmake -B build -S .
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
