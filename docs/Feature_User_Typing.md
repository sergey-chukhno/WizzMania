# Feature Analysis: User Typing Indicator

## 1. Implementation Mechanics
The User Typing Indicator is a real-time UX feature that displays "User is typing..." in the chat window when the remote party is actively pressing keys.

**Core Components:**
- **Signal Hooking:** In `ChatWindow.cpp`, we connect the Qt native `QLineEdit::textChanged` signal to our custom `onMessageTextChanged()` slot. Every time the user adds or deletes a character, this slot fires.
- **State Throttling (Debouncing):** We maintain a boolean `m_isTyping` flag and a `QTimer` (e.g., `m_typingTimer`). 
  - If `m_isTyping` is false when a key is pressed, we set it to true, start the timer for ~2 seconds, and emit a `PacketType::TypingIndicator` (value: `true`) to the server.
  - Every subsequent keypress resets the 2-second timer.
  - If the user stops typing for 2 full seconds, the `QTimer` expires, firing a slot that flips `m_isTyping` to false and emits a `PacketType::TypingIndicator` (value: `false`) to the server.
- **Server Routing:** The `TcpServer` receives this tiny boolean packet and instantly forwards it to the target user without database persistence.
- **UI Rendering:** The remote client receives the packet and conditionally shows or hides the "Typing..." `QLabel` overlay.

---

## 2. Advantages of the Architecture
- **Extremely Low Footprint:** A boolean status packet requires fewer than 10 bytes of network payload. Millions of these can be routed per second by the Reactor `select()` loop without straining server bandwidth.
- **Debounced Efficiency:** By utilizing the `QTimer` resetting mechanism, we do *not* send a network packet for every single keystroke. We send exactly one packet when typing begins, and one packet when it stops. This prevents hammering the TCP socket with hundreds of packets while the user types a sentence.
- **Enhanced UX Presence:** Real-time feedback drastically reduces "message collision" (where two users type long messages at the same time and hit send simultaneously, rendering the conversation disjointed).

---

## 3. Drawbacks and Limitations
- **State Desynchronization (The Ghost Typing Bug):** Because this feature relies on explicit `true` and `false` packets, it is highly vulnerable to network interruptions. 
  - Scenario: User A starts typing, User B receives `true`. User A's internet suddenly drops, or their computer crashes, preventing the `false` packet from ever being sent. User B will see "User A is typing..." infinitely until User A logs back in. 
- **Privacy Concerns:** Some users prefer high privacy and do not want senders to know when they are actively composing a message (or revising it multiple times). The current implementation lacks an explicit "Disable Typing Receipts" configuration toggle.

---

## 4. Future Roadmap
To solve the Ghost Typing bug, the recipient client (`ChatWindow`) should not rely exclusively on the `false` packet from the sender. The recipient itself should also run a local watchdog `QTimer`. If it receives a "typing: true" packet but does not receive another packet or an actual Chat Message within 15 seconds, the local UI should automatically hide the "Typing..." indicator as a fail-safe.
