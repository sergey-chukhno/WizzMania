# WizzMania Messaging Features (UX) 📲💬

WizzMania goes beyond basic text transfer by implementing real-time presence and interactive UX features.

---

## 1. User Typing Indicator
The Typing Indicator provides real-time feedback when the remote party is actively composing a message.

### 1.1 Mechanics & Debouncing
Instead of sending a packet for every keystroke, the client uses a **Debounce Logic**:
- **Timer**: A `QTimer` is started on the first `textChanged` signal.
- **Packet 1 (True)**: Sent only when the user *starts* typing.
- **Packet 2 (False)**: Sent only when the user *stops* typing for a full 2 seconds.
- **Benefit**: Drastically reduces network traffic while maintaining the "real-time" feel.

---

## 2. Presence & Status Sync
Presence is managed globally by the Server's `SessionManager`. 
- **Status Updates**: When a user changes their status (Online, Busy, Away), the server instantly broadcasts this to all of their friends.
- **Verification**: The Server enforces status rules (e.g., you cannot "Wizz" a user who is marked as `Busy`).

---

## 3. The "Nudge" (Buzz)
A homage to classic instant messengers, the Nudge is a protocol-level command that triggers a visual and behavioral effect on the recipient's machine.

### 3.1 Implementation
1.  **Trigger**: User clicks the "Nudge" button.
2.  **Packet**: `PacketType::Nudge` is routed by the Server.
3.  **Effect**: The recipient's `ChatWindow` performs a high-frequency layout jitter (shaking the window) and optionally plays a sound.

---

## 4. Architectural Advantages
- **Reactive UI**: Using Qt's Signal/Slot mechanism, the UI remains perfectly synchronized with the network state without polling.
- **Server Enforcement**: The Server validates interactions (e.g., status checks) before routing, preventing "harassment" or illegal state transitions.
