# WizzMania Evolution Roadmap: From Messenger to Life-Platform

This roadmap outlines the path to transforming WizzMania into a premium, secure, and innovative messaging solution.

## Phase 1: Security Hardening & Foundation (The "Shield" Phase)
*Focus: Protecting the server and establishing the core privacy promise.*

1. **[NEW] Global Rate Limiting**:
   - Implement token-bucket filtering in `TcpServer` to prevent brute-force login attempts and message spam.
   - Guard database resources from exhaustion.
2. **[S1] Zero-Knowledge Relay (Core E2EE)**:
   - Implement the X3DH key agreement and Double Ratchet algorithm using `libsignal` or a native C++ alternative like `Noble`.
   - Ensure the server acts only as a blind storage for encrypted blobs.
3. **[I5] Reactive Glassmorphism (UI Foundation)**:
   - Refactor the styling engine to support dynamic blur and gradient updates based on app state/activity.

## Phase 2: Social & Social Gaming (The "Hub" Phase)
*Focus: Increasing user stickiness and engagement.*

1. **[I4] ArcadeHub & Matchmaking**:
   - Create a central "Games" tab.
   - Implement server-side matchmaking and "Spectate" packets.
   - Expand beyond TicTacToe (e.g., Chess, Naval Battle).
2. **[I2] LivePresence (Rich Presence)**:
   - Build a local WebSocket/Socket gateway for external apps to push status.
   - Sync "What I'm Doing" across the network in real-time.

## Phase 3: Intelligence & Assistance (The "Brain" Phase)
*Focus: Differentiating from competitors with local, private intelligence.*

1. **[I1] WizzNeural (Local Client AI)**:
   - Integrate `llama.cpp` for local inference.
   - Build "Context Summaries" that analyze the active chat window (client-side only).
   - Smart replies based on conversation local history.

## Phase 4: Extreme Privacy & Sovereignty (The "Ghost" Phase)
*Focus: Catering to users with the highest security requirements.*

1. **[I3] ShadowChats**:
   - Add a high-security mode for ephemeral, self-destructuring conversations.
   - Uses separate key-pairs and zero-persistence memory.
2. **[S4] Traffic Padding (Metadata Obfuscation)**:
   - Inject dummy packets at regular intervals to mask real communication patterns.
   - Defeat timing-attack analysis.
3. **[S5] Sovereign Identity (DID/GPG)**:
   - Allow users to sign messages with their own private keys.
   - Support for decentralized identity providers.

---

## Next Immediate Steps (Proposed)
1. **Implementation of Rate Limiting**: Basic server protection.
2. **Start of Zero-Knowledge Refactor**: Moving away from plaintext message storage.
