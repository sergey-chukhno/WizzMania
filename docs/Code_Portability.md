# Engineering Standard: Code Portability

## 1. The Challenge
Wizz Mania is developed on macOS (Unix-like) but must be compilable and runnable on Windows by collaborators.
Operating Systems handle Networking APIs differently, specifically the Berkeley Sockets API.

---

## 2. Networking Abstractions
We encapsulate platform differences using Preprocessor Directives (`#ifdef`).

### A. Headers
*   **Unix/Mac:** Uses `<sys/socket.h>`, `<netinet/in.h>`, `<unistd.h>`.
*   **Windows:** Uses `<winsock2.h>` (part of the Windows API).

### B. Types
*   **Unix:** A socket is a file descriptor, represented as an `int`.
*   **Windows:** A socket is a `UINT_PTR` handle, represented as `SOCKET`.
*   **Solution:** We define a `SocketType` alias in `TcpServer.h` to unify this.

```cpp
#ifdef _WIN32
    typedef SOCKET SocketType;
#else
    typedef int SocketType;
#endif
```

### C. Function Calls
Most functions (`socket`, `bind`) are compatible, but closing a socket differs:
*   **Unix:** `close(fd)`
*   **Windows:** `closesocket(fd)`
*   **Initialization:** Windows requires `WSAStartup()` to be called before any networking.

---

## 3. Build System (CMake)
We use CMake to automate linking dependencies.

*   **Path Handling:** We treat all paths as forward slashes (`/`), which CMake automatically translates to backslashes (`\`) on Windows.
*   **Libraries:** Windows requires linking against `ws2_32.lib`.

```cmake
if(WIN32)
    target_link_libraries(wizz_common PUBLIC ws2_32)
endif()
```
This ensures Windows developers can build the project by just running `cmake --build .` without installing extra tools.
