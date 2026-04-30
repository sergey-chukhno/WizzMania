# Slide 12: Innovations & Route à Suivre (Roadmap)

## 1. Feature Overview
The final slide looks to the future. It divides upcoming work into 4 thematic phases: **The Shield** (Security), **Social Hub** (Community), **The Intelligence** (AI), and **The Ghost** (Privacy).

## 2. How it Works (The 4 Pro Pillars)
- **Pillar 1: The Shield**: Beyond simple relaying. Implementing **Double Ratchet** (E2EE) and **Sovereign Web3 IDs** (Ed25519) where your chat key is your wallet key.
- **Pillar 2: The Neural**: High-utility local AI. Native `llama.cpp` for **Real-time Summarization** and **Semantic Search** through vector indexing.
- **Pillar 3: The Ghost**: **BLE Mesh FIRST**. Off-grid transport for true resilience. Refactoring for **Zero-Copy** networking for absolute O(1) scale.
- **Pillar 4: The Shell**: **Futuristic Glassmorphism**. Moving beyond standard themes to a semi-transparent, modern mobile-style aesthetic.

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
