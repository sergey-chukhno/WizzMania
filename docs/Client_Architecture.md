# Client Architecture (Qt & C++)

## 1. Threading Model: Asynchronous Main Thread
Unlike the Server, which uses `select()` for IO multiplexing, the Client uses **Qt's Event Loop**.

### Design Choice: `QTcpSocket` on Main Thread
*   **Mechanism:** `QTcpSocket` is non-blocking. It emits the `readyRead()` signal when data arrives.
*   **The Flow:**
    1.  Main Loop (`QApplication::exec()`) handles UI events (clicks, painting).
    2.  OS notifies Qt of incoming TCP data.
    3.  Qt emits `readyRead()`.
    4.  Our `NetworkManager::onDataReceived()` slot runs, reads the data, constructs a Packet, and returns.
    5.  Main Loop resumes handling UI.
*   **Why Async?**
    *   **Responsiveness:** Blocking functions (`recv()`) would freeze the UI (Application Not Responding).
    *   **Simplicity:** No need for `std::thread` synchronization (Mutexes) typically needed when a background thread tries to update the UI.

### Architectural Pattern: The Observer
We are applying the **Observer Pattern** using Qt's Signal & Slot mechanism.
*   **Subject (Observable):** `NetworkManager`. It has state (Connection Status, Incoming Packets) and notifies observers when this state changes.
*   **Observers:** `LoginDialog`, `MainWindow`. They subscribe to signals to react to changes without polling.
*   **Benefit:** Loose coupling. `NetworkManager` doesn't know *who* is listening, only that it has broadcast an event.

### Senior Analysis: Why not Blocking I/O?
*   **Q: What happens if we put a `while(true)` loop (or `recv()`) inside a Button Click?**
    *   **A: UI Responsiveness.** In a blocking model, if only 5 bytes arrive, we must sleep and wait for the rest. In Async, we buffer the 5 bytes and **return to the Event Loop**. The user can continue to switch tabs or type while we wait for the rest of the packet.

### Senior Analysis: Dialog Flows & Security
*   **Q: Why `QDialog::exec()` instead of `show()`?**
    *   **A: Nested Event Loops.** `exec()` spins a *new, local* event loop. This blocks the *code execution* in `main()` (pausing the next lines) but keeps the *UI responsive*. It allows us to write linear, easy-to-read logic: `if (dialog.exec() == Accepted) { startApp(); }`.
*   **Q: Is `QString` secure for passwords?**
    *   **A: No.** `QString` (and `std::string`) uses Copy-on-Write and heap allocation. The password might be copied to multiple memory locations and not zeroed out immediately after use, making it vulnerable to RAM dumping.
    *   **Mitigation:** In high-security apps, we use a `SecureString` class that pins memory (prevents swapping to disk) and explicitly overwrites memory with zeros (`memset`) in its destructor. For this MVP, `QString` is acceptable, but we acknowledge the risk.

## 2. The `NetworkManager` Pattern (Implemented Step 2)
We do **not** use `QTcpSocket` directly inside UI classes (`LoginDialog`, `MainWindow`).

### Why? (Separation of Concerns & Lifetime)
1.  **Lifetime Management:** `LoginDialog` is destroyed after the user logs in. If it owned the socket, the connection would close. `NetworkManager` persists for the app's entire lifecycle.
2.  **Decoupling:** The UI should only care about *Logic* events ("User Logged In", "New Message"), not *Network* details ("Packet 202 received", "Byte buffer").
3.  **Single Point of Truth:** Only one class handles serialization/deserialization.

### Architecture: The Singleton Service
Since the Network is a global resource required by multiple isolated windows, we implement it as a **Singleton**.
*   **Access:** `NetworkManager::instance()` returns the static unique instance.
*   **Ownership:** It owns the `QTcpSocket` and the `std::vector<uint8_t>` receive buffer.

### API Surface
*   **Signals (Output):**
    *   `connected()`: Emitted when TCP handshake completes.
    *   `packetReceived(Packet)`: Emitted when a full binary packet is reassembled from the stream.
    *   `errorOccurred(QString)`: Emitted on socket errors or parsing failures.
*   **Slots (Input):**
    *   `connectToHost(host, port)`: Initiates the connection.
    *   `sendPacket(Packet)`: Serializes and writes data to the socket.

## 3. Signal/Slot Flow (Login Example)
1.  User clicks "Login" in `LoginDialog`.
2.  `LoginDialog` calls `NetworkManager::instance().connectToHost(...)`.
3.  `QTcpSocket` emits `connected()`.
4.  `NetworkManager` emits `connected()`.
5.  `LoginDialog` (listening) enables the "Submit" button.
6.  User clicks "Submit". `NetworkManager` sends `LoginPacket`.
7.  ... Server responds ...
8.  `NetworkManager` buffers data -> parses `Packet` -> emits `packetReceived`.
9.  `LoginDialog` checks packet type. If `LoginSuccess`, it closes itself.
