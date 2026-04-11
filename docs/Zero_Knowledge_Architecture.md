# 👻 Ghost Phase: End-to-End Encryption (Zero-Knowledge Relay)

## 1. Overview
The **Shield Phase** transitions WizzMania from a standard plaintext TLS messenger into a cryptographically secure, "Zero-Knowledge" relay. Utilizing the **Signal Protocol** (X3DH and Double Ratchet via `libsignal`), the server is mathematically blinded. It stores, routes, and delivers ciphertext blobs that it cannot decrypt, guaranteeing Perfect Forward Secrecy and Post-Compromise Security.

## 2. Server Key Infrastructure (Phase 1)
For the X3DH Key Agreement to function, Alice needs Bob's Public Keys before she can send her first encrypted message (even if Bob is offline). To facilitate asynchronous communication, the WizzMania Server acts as a central **Public Key Directory**.

### 2.1 The Key Registry
To support this, `DatabaseManager.cpp` has been expanded to securely hold the foundational E2EE structures:
- **`public_keys` Table**: Maps a `USERNAME` to their base64-encoded `IDENTITY_KEY`, `SIGNED_PRE_KEY`, and a highly secure `SIGNED_PRE_KEY_SIG`.
- **`one_time_keys` Table**: A disposable pool of cryptographic keys uploaded by the client. These guarantee perfect forward secrecy on the initial handshake.

### 2.2 Key Depletion & Retrieval
When Alice requests Bob's Key Bundle (`fetchPreKeyBundle`), the server efficiently performs an atomic query sequence:
1. Retrieve Bob's `IDENTITY_KEY` and `SIGNED_PRE_KEY`.
2. Retrieve the *first available* One-Time Pre-Key from the `one_time_keys` table.
3. Automatically **DELETE** the consumed One-Time Pre-Key to guarantee cryptographic uniqueness and prevent replay attacks.
4. If the One-Time Key pool is exhausted, the server gracefully returns the bundle with a flag indicating depletion, allowing the Signal Protocol fallback logic to proceed using just the Signed Pre-Key.

## 3. Pending Implementation
* **Crypto Handlers:** TCP packet handlers to allow clients to `UploadPreKeys` and `FetchPreKeyBundle`.
* **Client Cryptography:** Embedding `libsignal-protocol-c` into the frontend to perform X3DH handshakes and AES-GCM encryption before TCP transmission.
