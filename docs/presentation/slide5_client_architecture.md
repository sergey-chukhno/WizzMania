# Slide 5: Architecture du Client (Deep Dive)

## 1. Feature Overview
The client architecture bridge the gap between high-speed networking and responsive UI. It features the **GameBridge** (IPC poller), **NetworkManager** (the TLS singleton), and **ChatWindow** (the View).

## 2. How it Works
- **Thread Isolation**: The `NetworkManager` lives on its own `QThread`. This ensures that even if the UI freezes (e.g., during a heavy window resize), the networking remains active and packets aren't dropped.
- **Signal/Slot Dispatch**: When a packet arrives, `NetworkManager` emits a signal. Any interested UI component (like `ChatWindow`) "listens" to this signal to update itself.

## 3. Why This Code (Rationale)
- **Qt Event Loop**: We leverage Qt's event-driven nature. `NetworkManager::onReadyRead` is called whenever the OS has data for us.
- **Bridge Pattern**: `GameBridge` acts as the intermediary. It doesn't contain game logic; it only converts raw memory data into high-level Qt signals like `localGameStatusChanged`.

## 4. How the Code Implements This
### `GameBridge::onPollGameIPC()`
This is a **Polling Loop** (usually 30-60Hz). It locks the POSIX semaphore, reads the `isPlaying` flag, compares it to the previous state, and emits a signal only if something changed. This prevents "Signal Storms" (emitting thousands of unnecessary updates).
### `NetworkManager::instance()`
A **Threaded Singleton**. We use `moveToThread()` to move the manager object to a dedicated worker thread. This is a powerful C++ technique to keep the "Main Thread" (UI thread) clean.
### `ChatWindow::shake()`
A demonstration of Qt's animation capabilities—showing that WizzMania is a "fun" consumer app, not just a technical demo.

## 5. Strategic Analysis

### Advantages
- **Responsiveness**: The UI never "stutters" due to network activity.
- **Modular Events**: Adding a new feature (like a "Game Starting" notification) just requires connecting a new function to an existing signal.

### Drawbacks / Limits
- **Context Switching**: Passing signals between threads in Qt (Queued Connections) involves a small overhead. For 99% of UI tasks, this is negligible.

### Edge Cases
- **Semaphore Deadlock**: If a game locks the IPC semaphore and then crashes, the `GameBridge` will hang in `lock()`. We solve this by using **timed locks** or robust semaphores (which clean up on process exit).
- **Socket Disconnection**: If the network thread dies, the UI must be notified immediately to gray out the controls.
