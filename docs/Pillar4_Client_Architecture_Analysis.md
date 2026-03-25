# Pillar 4 Mentorship: Client Architecture Analysis

This document analyzes the architectural decisions, strengths, and weaknesses of the WizzMania Qt6 graphical client application. While the server is designed for massive I/O concurrency, the client's architecture is optimized for user responsiveness, crash isolation, and framework integration.

---

## 1. The Core: The Qt Event Loop (`QApplication::exec()`)

**Architecture:**
Unlike our custom C++ `TcpServer` which relies on a raw `select()` event loop, the client perfectly integrates with the massive, industry-standard **Qt Event Loop**. When `m_app.exec()` is called, Qt takes over, continuously polling the OS for mouse clicks, keyboard inputs, paint events, and network packets seamlessly on a single main thread.

**Advantages:**
- **Zero Lock Contention:** Because network packets (`NetworkManager::onReadyRead`) and UI updates (like appending a message to `ChatWindow`) both execute sequentially on the exact same thread, we completely avoid Mutexes, Locks, and Race Conditions. The UI is inherently thread-safe.
- **Signal & Slot Pattern:** The client heavily utilizes Qt's loosely coupled `emit signal()` mechanism. For example, `NetworkManager` parses a packet and emits `messageReceived(user, text)`. It doesn't know *who* handles it. `ChatWindow` listens for that signal un-intrusively. This is a textbook implementation of the **Observer Pattern**.

**Drawbacks:**
- **"The Freezing Window":** The fatal weakness of a shared UI/Network event loop is that **any blocking operation freezes the entire application.** If `AvatarManager` synchronously downloads and decodes a massive 50MB animated avatar on the main thread, the user's chat window will completely freeze, the mouse will turn into a loading spinner, and the OS will declare the app "Not Responding" until the operation finishes. 
- **Future Solution:** Heavy I/O bound tasks (like loading massive game assets or encrypting audio buffers) must eventually be moved to a `QThread` or `QtConcurrent::run`, with only the final output emitted back to the main thread for rendering.

---

## 2. Networking Layer: Open-Closed Principle (OCP)

**Architecture:**
The `NetworkManager` is decoupled from the UI. More importantly, we recently refactored its packet parsing logic to utilize a **Dispatch Table** pattern (`QHash<PacketType, std::function>`).

**Advantages:**
- **Closed for Modification, Open for Extension:** Previously, developers had to modify a massive `if-else if` block inside `onReadyRead()` to add new features. Now, to add a "Video Call" feature, a developer simply calls `registerHandler(PacketType::VideoCall, lambda)` during initialization. The core routing logic never needs to be touched again.
- **Testability:** Because handlers are lambdas or bound functions, we can trivially Unit Test `handleContactListPacket` by passing it a fake packet buffer, completely without launching the graphical UI.

**Drawbacks:**
- **Payload Validation Blindspots:** While the Dispatch Table routes packets cleanly, we currently trust that every packet is identically structured. Malformed packets from malicious users could crash the client if we attempt to read a String where an Integer should be.

---

## 3. Game Integration: Process Isolation (`QProcess`)

**Architecture:**
The most critical architectural decision in WizzMania is how games are launched. Instead of compiling `TileTwister` (SDL2) and `BrickBreaker` (SFML) *into* the Qt executable as dynamically linked libraries, we execute them as entirely separate, completely isolated operating system processes using `QProcess::startDetached()`.

**Advantages:**
- **Perfect Crash Isolation:** C++ games are notoriously prone to memory leaks and Segmentation Faults (accessing null pointers). If a player's `TileTwister` game crashes, the OS violently kills the game process. However, because it is isolated from the WizzMania client process, the user's chat session remains completely uninterrupted.
- **Framework Agnostic:** WizzMania is built in Qt. TileTwister is built in SDL2. BrickBreaker is built in SFML. By isolating them as processes, we completely avoided catastrophic CMake dependency hell and framework rendering conflicts. 

**Drawbacks:**
- **Zero Inter-Process Communication (IPC):** Because the games are isolated processes, WizzMania cannot easily talk to them natively. We cannot easily build a feature where "WizzMania displays your live BrickBreaker score" or "TileTwister automatically invites the person you are chatting with to a multiplayer match." 
- **Future Solution:** To bridge this gap natively, we would need to implement advanced **Shared Memory (`QSharedMemory`)** or run local RPC/WebServers inside the game engine that the Wizz Client polls for data.

---

## 4. Resource Management: RAII vs The Qt Object Tree

**Architecture:**
C++ relies heavily on RAII (Resource Acquisition Is Initialization) via smart pointers to prevent memory leaks. However, Qt relies exclusively on its "Parent-Child Object Tree" (where passing `this` to a QWidget's constructor ensures Qt deletes the child when the parent dies).

**Advantages:**
- **Automatic Garbage Collection:** We rarely write `delete` in our GUI code. When `MainWindow` closes, it recursively deletes `ChatWindow`, which deletes all the `QPushButton` widgets automatically.

**Drawbacks:**
- **Dangling Pointers in Callbacks:** Mixed memory models are dangerous. We currently use `std::unique_ptr` for audio streams and raw pointers for QWidgets. If a background network callback triggers *after* a `ChatWindow` has been deleted by Qt, the pointer we captured in our lambda will cause a catastrophic Use-After-Free crash. We mitigated this by heavily leveraging Qt's automatic connection deletion in `QObject::connect`, but careful lifetime analysis is always required.
