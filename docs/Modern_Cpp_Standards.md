# Engineering Standard: Modern C++ & Safety

## 1. Principles
Wizz Mania adheres to **Modern C++ (C++17)** standards to prevent memory leaks, crashes, and undefined behavior.

---

## 2. RAII (Resource Acquisition Is Initialization)
Any resource (Heap Memory, File Handle, Network Socket) must be owned by an object. When the object goes out of scope, the resource is automatically released.

### Example: TcpServer
*   **Resource:** The Server Socket (`m_serverSocket`).
*   **Risk:** If `start()` throws an exception, `stop()` might never be called, leaking the port.
*   **Our Solution:** The Destructor `~TcpServer()` unconditionally closes the socket.
    ```cpp
    TcpServer::~TcpServer() {
        if (m_serverSocket != INVALID) close(m_serverSocket);
    }
    ```

---

## 3. Memory Management
We strictly **forbid** manual memory management (`new`/`delete`, `malloc`/`free`).

### Containers
*   **Buffers:** Use `std::vector<uint8_t>` instead of `char*`. This handles resizing and deletion automatically.
    *   *See `common/Packet.h`.*
*   **Strings:** Use `std::string` instead of `char*`.

### Copy Semantics
We explicitly delete copy constructors for classes that own unique resources to prevent "Double Free" errors.

```cpp
// server/TcpServer.h
TcpServer(const TcpServer&) = delete;
TcpServer& operator=(const TcpServer&) = delete;
```
This forces the compiler to produce an error if we accidentally try to copy the server logic.

---

## 4. Exception Handling
We do not use C-style Error Codes (return -1) for logic errors. We use Exceptions.
*   **Benefits:** Impossible to ignore. If `readString` fails, the stack unwinds safely, destructors run (RAII), and the application can catch/log the error at the top level.

---

## 5. Data Structures & Algorithms
We choose containers based on **Big-O Complexity** and **Reference Stability**.

### Managing Sessions (The "Lookup" Problem)
*   **Requirement:** Find a connected user by their `SocketID` or `Username` instantly.
*   **Choice:** `std::unordered_map` (Hash Map).
    *   **Lookup Speed:** O(1) constant time.
    *   **VS Vector:** a `std::vector` would require O(N) linear search.
    *   **Reference Stability:** Unlike `std::vector`, adding new elements to a map does **not** invalidate pointers/references to existing elements. This is critical for preventing crashes when new users join while we are processing another user.
