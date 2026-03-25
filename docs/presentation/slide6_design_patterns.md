# Slide 6: Design Patterns & Principes OOP

## 1. Feature Overview
This slide is the "Senior Engineer" heart of the presentation. It lists 6 major design patterns that make WizzMania maintainable, scalable, and professional.

## 2. How it Works (The 6 Patterns)
1. **Command Pattern**: Decouples "What to do" (The Handler) from "Who sends it" (The Router).
2. **Service Layer**: Aggregates business logic (Session management) away from infrastructure (TCP).
3. **Actor Model**: Ensures thread-safety for data persistence (The Database worker).
4. **Adapter / Bridge**: Connects "Alien" systems (POSIX SHM) to "Native" systems (Qt Signals).
5. **Observer Pattern**: The core of Qt (Signals/Slots).
6. **Singleton Pattern**: Ensures a single source of truth for the global connection.

## 3. Why This Code (Rationale)
We apply **SOLID Principles**:
- **S (Single Responsibility)**: `TcpServer` only accepts. `PacketRouter` only routes.
- **O (Open/Closed)**: We can add a "Video Call" feature by adding a new `VideoHandler` without changing a single line of `TcpServer.cpp`.
- **D (Dependency Inversion)**: `GameBridge` depends on an abstraction (`NativeSharedMemory`), not a concrete game implementation.

## 4. How the Code Implements This
### `PacketRouter::registerHandler`
Instead of a 500-line `switch` statement, we have a `std::unordered_map<PacketType, std::unique_ptr<IPacketHandler>>`. This is the **Command Pattern**. It allows us to unit test each handler in total isolation (see Slide 10).
### `NetworkManager::instance()` (Meyer's Singleton)
Ensures thread-safe initialization. Notice the `moveToThread` call—this ensures the singleton's events happen on the correctly assigned worker thread.

## 5. Strategic Analysis

### Advantages
- **Maintainability**: New developers don't need to understand the whole system; they only need to understand one "Handler" to add a feature.
- **Testability**: Highly decoupled code is 10x easier to Mock and Unit Test.

### Drawbacks / Limits
- **Indirection**: More classes and files can make the initial learning curve steeper.
- **Abstraction Cost**: Virtual function calls (vtable) have a tiny runtime cost, but in C++, this is negligible compared to the architectural benefits.

### Edge Cases
- **Double-init Singleton**: Standard Meyer's singleton is safe, but custom threaded ones (like ours) must be careful about race conditions during the very first access.
- **Handler Collision**: We must ensure no two handlers register for the same `PacketType`, or handle the error gracefully during startup.
