# WizzMania Documentation Hub 🛡️🏗️

Welcome to the central technical documentation for WizzMania. This repository is organized into four logical pillars to provide a clear understanding of the project's architecture, features, and operational requirements.

---

## 🗺️ Documentation Sitemap

### [01. Architecture](./01_Architecture/Server.md)
The blueprint of our high-concurrency ecosystem.
- [Server & Protocol](./01_Architecture/Server.md): Standalone Asio, binary framing, and thread isolation.
- [Client & Threading](./01_Architecture/Client.md): Qt Worker Object pattern and safe async messaging.
- [Security & E2EE](./01_Architecture/Security.md): TLS 1.2, Signal Protocol (Double Ratchet), and Shield protection.
- [IPC & Games Integration](./01_Architecture/IPC_&_Games.md): Native shared-memory synchronization and game-core isolation.

### [02. Features](./02_Features/Messaging.md)
Specifications for the user-facing capabilities of the messenger.
- [Reactive Messaging](./02_Features/Messaging.md): Typing indicators, presence sync, and the "Nudge".
- [Multimedia Exchange](./02_Features/Multimedia.md): Voice messaging (PCM/zlib) and Avatar management.

### [03. Operations](./03_Operations/Environment_Setup.md)
Guides for developers and administrators.
- [Environment Setup](./03_Operations/Environment_Setup.md): Build instructions for WSL, Windows, and macOS.
- [Testing & Verification](./03_Operations/Testing_&_Verification.md): Guide to the 77-test verification suite and security coverage.

### [04. Strategy](./04_Strategy/Roadmap_&_Standards.md)
The vision and rules governing the project.
- [Innovation Roadmap](./04_Strategy/Roadmap_&_Standards.md#2-long-term-innovation-roadmap): From Messenger to Life-Platform.
- [Engineering Standards](./04_Strategy/Roadmap_&_Standards.md#1-engineering-standards-modern-c17): RAII, Safe Memory, and C++17 principles.

---

## 🏛️ [Archive](./archive/)
For historical reference, including initial audits and mentorship Q&A sessions.
- [Historical Audits & QA](./archive/)

---

## 🚀 Getting Started
To build the full project including all games and the test suite:
```bash
cmake -B build -S .
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

