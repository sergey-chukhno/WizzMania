# Slide 12: Innovations & Route à Suivre (Roadmap)

## 1. Feature Overview
The final slide looks to the future. It divides upcoming work into 4 thematic phases: **The Shield** (Security), **Social Hub** (Community), **The Intelligence** (AI), and **The Ghost** (Privacy).

## 2. How it Works (The 4 Phases)
- **Phase 1: The Shield**: Focus on "Green C++" — optimizing memory usage via its zero-copy architecture and adding E2E encryption.
- **Phase 2: Social Hub**: Integrating more games (ArcadeHub) and adding Web3-based tipping via Lightning Network.
- **Phase 3: The Intelligence**: Local AI assistant using `llama.cpp` to summarize chats without sending data to the cloud.
- **Phase 4: The Ghost**: Mesh networking for chat without internet (Bluetooth/P2P) and self-sovereign identity (DID).

## 3. Why This Code (Rationale)
- **Zero-Copy**: In C++, we can pass pointers or `std::string_view` instead of copying data. This makes WizzMania "Ecological" because it uses less CPU/RAM for the same task.
- **`llama.cpp` Integration**: WizzMania is built in C++, and so is the world's most popular local LLM engine (`llama.cpp`). Integration is native and requires no complex wrappers.

## 4. How the Code Implements This
### Upcoming: `PacketType::EncryptedMessage`
We will introduce a new handler that handles AES-GCM encrypted payloads.
### Upcoming: `WizzNeural`
A class wrapping the Llama context, running on a dedicated thread (Actor pattern again!) to provide "Smart Replies" based on the last 10 messages of a conversation.

## 5. Strategic Analysis

### Advantages
- **Vision**: Shows that WizzMania isn't just a school project but a platform with a long-term growth strategy.
- **Competitive Edge**: Local AI and Mesh networking are high-value "Hard C++" features that generic apps (like Electron-based Discord) struggle to implement efficiently.

### Drawbacks / Limits
- **Ambitious**: This roadmap requires significant R&D, especially for Mesh and sovereign identity.

### Edge Cases
- **Hardware Limitations**: `llama.cpp` requires at least 4GB of RAM for decent performance. We must detect hardware capabilities before activating "Intelligence" features.
- **Regulatory changes**: E2EE (End-to-End Encryption) and Anonymity are legally complex areas in certain jurisdictions.
