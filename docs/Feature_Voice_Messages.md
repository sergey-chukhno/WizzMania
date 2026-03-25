# Feature Analysis: Voice Messages

## 1. Implementation Mechanics
The Voice Message feature in WizzMania allows users to record audio via their microphone and send it natively to an active chat window. 

**Core Components:**
- **Capture & Playback:** We utilize Qt6's native Multimedia module, specifically `QAudioSource` to capture raw PCM audio from the default microphone, and `QAudioSink` to play the received audio buffer back to the user's speakers.
- **Buffer Management:** During recording, `QIODevice` continuously streams the raw microphone bytes into a `QByteArray` acting as an in-memory buffer.
- **Compression & Transport:** Before transmission, the raw `.wav` data is compressed using Qt's `qCompress()` algorithm (zlib). The data is then packed into a custom `PacketType::VoiceMessage` which includes the target user, the audio duration, and the binary payload.
- **Routing:** The `TcpServer` receives the packet, identifies the target `ClientSession`, and routes the binary payload without needing to inspect or decode the audio data.

---

## 2. Advantages of the Architecture
- **Cross-Platform Abstraction:** By leaning on `QAudioSource/Sink`, Qt handles the nightmare of interfacing with Windows WASAPI, macOS CoreAudio, and Linux ALSA/PulseAudio. Our C++ codebase remains perfectly OS-agnostic.
- **Zero Third-Party Codec Dependencies:** By utilizing raw PCM audio + `qCompress`, we eliminated the need to compile complex external C libraries like `libopus`, `libvorbis`, or `ffmpeg` into our CMake build, drastically simplifying the build process.
- **UI Decoupling:** The `AudioManager` class handles the asynchronous audio threads and buffer allocations completely independently of the `ChatWindow` UI thread, preventing the application from freezing during playback.

---

## 3. Drawbacks and Limitations
- **Lack of Streaming (Store-and-Forward):** Currently, a user must finish recording their entire 60-second voice note, wait for it to compress, and send it as one massive chunk. The recipient must wait for the entire 10MB packet to arrive before playback begins. Production apps (like WhatsApp or Discord) use chunked UDP streaming, allowing the recipient to listen to the first second of audio while the rest is still downloading.
- **Inefficient Codec Usage:** `qCompress` (zlib) is designed for compressing text and generic binary data, not audio frequencies. It is incredibly inefficient compared to purpose-built audio codecs like **Opus**. A 1-minute voice message in WizzMania consumes vastly more bandwidth than a 1-minute message on Telegram.
- **Memory Pressure:** Storing the entire raw audio buffer in RAM during recording means a user who leaves the app recording for an hour could cause the client to crash due to heap exhaustion (OOM).

---

## 4. Future Roadmap
To scale this feature, the immediate next step is integrating **libopus** via CMake FetchContent. We would encode the audio stream on-the-fly into Opus packets and transport them in 20ms chunks over a UDP socket rather than a TCP stream, guaranteeing ultra-low latency playback.
