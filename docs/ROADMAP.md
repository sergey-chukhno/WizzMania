# Wizz Mania — Project Roadmap & Feature Specification

> **Status:** Draft for Approval
> **Objective:** Production-grade C++ TCP/IP Chat Application with Qt GUI.

---

## 1. Feature Specification

### A. Essential Features (Required)
*These are the non-negotiable requirements to satisfy the core project constraints.*

#### 1. Networking Core (TCP)
*   **Centralized Server:**
    *   Must handle multiple concurrent client connections (non-blocking or multi-threaded).
    *   Must maintain a list of active sessions.
    *   Must broadcast status updates (Online/Offline) to relevant peers.
*   **Protocol:**
    *   Binary or JSON-based packet structure (Header + Payload).
    *   Robust serialization/deserialization.
    *   Handling of packet fragmentation/reassembly (framing).

#### 2. Authentication & Identity
*   **Login/Register:** Username and password authentication.
*   **Session Management:** Prevent duplicate logins from the same user.
*   **Persistence:** Server must store user credentials (simple file-based or SQLite).

#### 3. Contact Management
*   **Friend List:** View a list of added contacts.
*   **Add/Remove:** Logic to request friendship or delete a contact.
*   **Status Sync:** Real-time visual indication of contact status (Online/Offline).

#### 4. Instant Messaging
*   **1-to-1 Chat:** Reliable delivery of text messages.
*   **Notifications:** Visual cue when a new message is received.
*   **History (Session-based):** Messages persist in the UI while the window is open.

#### 5. GUI (Qt Framework)
*   **Login Window:** Entry point for connection configuration (IP/Port) and Auth.
*   **Main Window (Buddy List):** The central hub displaying contacts and self-status.
*   **Chat Window:** Tabbed or separate windows for active conversations.

---

### B. Supplementary Features (Advanced)
*These features demonstrate senior-level engineering and add "polish". We will tackle these only after the core is stable.*

1.  **Rich Interactions ("Nudge" / "Buzz"):**
    *   A protocol command that shakes the recipient's chat window.
    *   Demonstrates control over window manager and custom protocol events.
2.  **Advanced Status System:**
    *   Custom states: *Busy, Away, Out to Lunch, Appear Offline*.
    *   Custom status messages (e.g., "Coding...").
3.  **Group Chat:**
    *   Ability to invite multiple users to a context.
    *   Server routing complexity increases significantly.
4.  **Typing Indicators:**
    *   Real-time "User is typing..." events sent efficiently (debounce logic required).
5.  **Offline Messaging:**
    *   Server stores messages for offline users and delivers them upon reconnection.
6.  **Visual Polish (Stylesheets):**
    *   Use Qt QSS to create a modern "Glass" or "Dark Mode" aesthetic.

---

## 2. Execution Roadmap (2 Weeks)

This schedule prioritizes **architecture and correctness** over speed.

### Week 1: The Engine Room (Networking & Logic)
*Goal: A fully functional console-based chat system.*

#### **Day 1: Foundation & Protocol**
*   **Morning:** Project Setup (CMake/QMake), Git init, Directory structure.
*   **Afternoon:** Protocol Design. Define `Packet` structure (Header: Type, Length; Body).
*   **Outcome:** A compilable project where we can serialize/deserialize a dummy packet.

#### **Day 2: The Server Core**
*   **Morning:** Implement `TcpServer` class. Handle `clean` vs `unclean` disconnections.
*   **Afternoon:** Implement `ClientSession` management. Design the "Session Map".
*   **Outcome:** A Server that accepts connections and logs them.

#### **Day 3: Authentication & Client Networking**
*   **Morning:** Implement `NetworkManager` for the Client (Socket handling).
*   **Afternoon:** Design and implement `Login` and `Register` flows (Server-side DB lookup).
*   **Outcome:** Client can connect, login, and fail gracefully if credentials are wrong.

#### **Day 4: Contact List Architecture**
*   **Focus:** Data Structures. How do we store dependencies?
*   **Tasks:** Implement `AddContact` / `RemoveContact` protocol commands. Server updates DB.
*   **Outcome:** Client receives a list of friends upon login.

#### **Day 5: Messaging Logic**
*   **Focus:** Routing.
*   **Tasks:** Implement `SendMessage` / `ReceiveMessage` flows. Handle "User Not Found" errors.
*   **Outcome:** Two console clients can exchange text.

#### **Day 6: Robustness & Refactoring**
*   **Focus:** Quality Assurance.
*   **Tasks:** Test packet fragmentation (simulate bad network). Refactor giant functions.
*   **Outcome:** A solid, crash-resistant networking layer.

#### **Day 7: Architecture Review (Buffer Day)**
*   **Tasks:** Catch up on delays. Review code against `rules.md` (RAII, Pointers).

---

### Week 2: The Face (GUI & User Experience)
*Goal: Seamless integration of Qt with our C++ core.*

#### **Day 8: Qt Integration & Login UI**
*   **focus:** Threading Model. Use `Signals & Slots` to bridge Network thread (or socket) to GUI thread.
*   **Tasks:** Build `LoginDialog`. Connect it to `NetworkManager`.
*   **Outcome:** A GUI that logs in and transitions to a (empty) main window.

#### **Day 9: The Buddy List (Main Window)**
*   **Focus:** Model/View.
*   **Tasks:** Implement `ContactListWidget`. displaying friends and their status icons (Green/Grey).
*   **Outcome:** Users see their friends come online in real-time.

#### **Day 10: Chat UI**
*   **Focus:** User Interaction.
*   **Tasks:** Build `ChatWindow`. Handle specific signals for incoming messages to open/focus the correct window.
*   **Outcome:** Full graphical chat capability.

#### **Day 11: Advanced Feature - Status & Polish**
*   **Focus:** "Supplementary Feature 2" (Custom Status).
*   **Tasks:** Add dropdown for status. Update protocol to carry status text.

#### **Day 12: Advanced Feature - Nudges & Fun**
*   **Focus:** "Supplementary Feature 1" (Nudge).
*   **Tasks:** Implement the "Shake" algorithm in Qt. Add the protocol trigger.

#### **Day 13: UI Styling (QSS)**
*   **Focus:** Aesthetics.
*   **Tasks:** Apply a dark/modern theme. Fix layout margins/spacing.

#### **Day 14: Final Delivery Prep**
*   **Tasks:** Code Cleanup (`clang-format`), README documentation, Build Instructions.
*   **Verification:** Full walkthrough of requirements.

---

## 3. Future Improvements (Phase 5 - Post MVP)

### High Priority
1.  **Worker Thread Pool (Database Offloading):**
    *   **Current:** Single Thread (Blocking).
    *   **Target:** Decouple IO from Logic. 4-8 Worker Threads handling DB queries.
    *   **Why:** Prevents UI freezes for other clients during heavy load.

2.  **Security Layer:**
    *   Client-side Password Hashing (SHA-256).
    *   SSL/TLS Encryption (OpenSSL).

---

## 4. Next Step

If you approve this Roadmap, we will begin with **Day 1: Foundation & Protocol**.

> **Pending Approval**
> Please reply with: **"I approve"**
