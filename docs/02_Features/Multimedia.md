# WizzMania Multimedia Features 🎙️🖼️

WizzMania supports rich media exchange through its custom binary protocol, handling large-scale binary data such as audio and images.

---

## 1. Voice Messaging
The Voice Message feature allows users to record, transmit, and play back native audio.

### 1.1 Technical Implementation
- **Hardware Abstraction**: Uses Qt's `QAudioSource` (Mic) and `QAudioSink` (Speakers) for cross-platform hardware support.
- **Buffer Mode**: PCM audio is captured into an in-memory `QByteArray`.
- **Compression**: Raw audio is compressed via `qCompress` (zlib) before being packed into a `PacketType::VoiceMessage`.
- **Threading**: `AudioManager` runs asynchronously to prevent GUI freezes during playback or recording.

---

## 2. Avatar Management
User identities are enhanced by custom avatars that are synchronized across the network.

### 2.1 Synchronization Flow
1.  **Selection**: User chooses an image (PNG/JPG).
2.  **Downscaling**: The client resizes the image to a standardized resolution (e.g., 256x256) to save bandwidth.
3.  **Transmission**: The image is sent as a raw binary blob via the Server's relay.
4.  **Caching**: Remote clients cache the avatar to disk to avoid re-downloading on every session.

---

## 3. Large Binary Transfers & Optimization
WizzMania's protocol supports packets up to **10MB** by default. To handle large multimedia payloads safely:
- **Fragmentation**: The server accumulates partial packets in a dedicated session buffer.
- **Memory Caps**: All handlers enforce a maximum allocation size to prevent memory-exhaustion (OOM) attacks from malicious clients sending oversized "mock" multimedia packets.

---

## 4. Future Roadmap (Streaming)
Currently, multimedia features follow a **Store-and-Forward** model (the whole file must be received before it can be used). 
- **Goal**: Implement **Chunked Streaming** for voice messages, allowing recipients to hear the start of a message while the rest is still transferring.
- **Codec**: Transition from zlib to **Opus** for specialized audio compression.
