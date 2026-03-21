# WizzMania Evolution Roadmap: From Messenger to Life-Platform

This roadmap outlines the path to transforming WizzMania into a premium, secure, and innovative messaging solution.

## Phase 1: Security Hardening & Foundation (The "Shield" Phase)
*Focus: Protecting the server and establishing the core privacy promise.*

1. **[NEW] Global Rate Limiting**:
   - Implement token-bucket filtering in `TcpServer` to prevent brute-force login attempts and message spam.
2. **[S1] Zero-Knowledge Relay (Core E2EE)**:
   - Implement X3DH and Double Ratchet. Ensure the server is a "blind blind repository."
3. **[GREEN] Zero-Copy Architecture**:
   - Optimize the C++ networking stack to use zero-copy buffers (e.g., `boost::asio::buffer` with pre-allocated pools) to minimize CPU cycles and energy per message.

## Phase 2: Social & Social Gaming (The "Hub" Phase)
*Focus: Increasing user stickiness and engagement.*

1. **[I4] ArcadeHub & Matchmaking**:
   - Server-side matchmaking for TicTacToe, Chess, and more.
2. **[WEB3] Value-Exchange Protocol**:
   - Integrate Lightning Network (Bitcoin) or Solana for instant peer-to-peer tipping and game wagers directly in-chat.
3. **[I2] LivePresence (Rich Presence)**:
   - Real-time "What I'm Doing" sync via local WebSocket gateway.

## Phase 3: Intelligence & Assistance (The "Brain" Phase)
*Focus: Differentiating from competitors with local, private intelligence.*

1. **[I1] WizzNeural (Local Client AI)**:
   - Integrate `llama.cpp` for local inference.
   - Build "Context Summaries" that analyze the active chat window (client-side only).
   - Smart replies based on conversation local history.

## Phase 4: Extreme Privacy & Sovereignty (The "Ghost" Phase)
*Focus: Catering to users with the highest security requirements.*

1. **[I3] ShadowChats**: Ephemeral conversations with zero-persistence.
2. **[MESH] Resilience Protocol (Internet-Free)**:
   - Support for Peer-to-Peer mesh connectivity via BLE and Wi-Fi Direct.
   - Off-grid communication when the global internet is restricted.
3. **[S4] Traffic Padding**: Masking communication patterns with dummy packets.
4. **[S5] Sovereign Identity (DID)**: Using cryptographic keys (GPG/ED25519) instead of phone numbers for identity.

---

## Next Immediate Steps (Proposed)
1. **Implementation of Rate Limiting**: Basic server protection.
2. **Start of Zero-Knowledge Refactor**: Moving away from plaintext message storage.
