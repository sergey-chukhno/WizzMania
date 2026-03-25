# WizzMania Cross-Platform Portability (WSL & Windows)

## Introduction
The WizzMania messenger application, along with its standalone games (`TicTacToe`, `TileTwister`, and `CyberpunkCannonShooter`), must be completely portable across different operating systems. Due to the multi-process IPC architecture, collaborators working on **Linux (WSL / Ubuntu)**, **Native Windows**, and **macOS** need to compile and launch the application seamlessly without manual hardcoded path interventions.

This document describes the design strategies and fixes in place that guarantee robust WSL and cross-platform portability.

## 1. Zero-Install Dependencies via CMake Content Fetch
One common issue with C++ games across various operating systems is broken framework dependencies (e.g., mismatched SFML versions, missing `sdl2_ttf` libraries on Windows vs macOS). WizzMania enforces local builds using `FetchContent` in `CMakeLists.txt` for all third-party libraries:

* **SFML 3.0 (TicTacToe & BrickBreaker):** Automatically pulls directly from the SFML GitHub release for the target platform.
* **SDL2 Ecosystem (TileTwister):** Automatically clones `SDL2`, `SDL2_image`, `SDL2_mixer`, and `SDL2_ttf`.
* **GoogleTest:** Cloned from the main repository.

Because `find_package()` failures instantly fallback to cloning these repositories into `build/_deps/`, a WSL user essentially needs zero prior global SDK installations. CMake compiles everything locally to generate identical output binaries across all operating systems.

## 2. Dynamic Executable Resolution (Path Portability)
Instead of forcing hardcoded absolute paths, the UI client (`GameLauncher.h`) climbs the directory tree dynamically relative to the Qt `QCoreApplication::applicationDirPath()`. 

To launch `TileTwister`, it utilizes `resolveExecutablePath("TileTwister")`, which iterates through common CMake Ninja output locations:
* `build/games/TileTwister/TileTwister`
* `build/bin/TileTwister`
* `build/bin/Debug/TileTwister`

### Automatic `.exe` Fallback
On native Windows platforms (`MSVC` / `MinGW`), the system compiles binaries with a `.exe` extension. The launcher automatically appends `.exe` and searches the binaries if `Q_OS_WIN` is defined:
```cpp
QString exeName = baseName;
#ifdef Q_OS_WIN
  exeName += ".exe";
#endif
```
This isolates pathing issues, meaning the exact same C++ logic powers `wizz_client` on a Mac and a Windows machine seamlessly.

## 3. Abstracted IPC Networking (POSIX vs Windows SYSV)
Because the games use shared memory to communicate, the hardest portability challenge was memory-mapping system calls. Linux/WSL relies exclusively on POSIX (`shm_open`, `mmap`), while Native Windows requires `CreateFileMappingA`.

To abstract this, the games embed the `wizz::NativeSharedMemory<T>` templated C++ class which conditionally compiles operating-system-specific commands:

### **On WSL / Linux / macOS (`#if defined(__APPLE__) || defined(__linux__)`)**
The memory block relies exclusively on POSIX headers:
* `<sys/mman.h>`, `<sys/stat.h>`, `<semaphore.h>`, `<fcntl.h>`
* **Instantiation:** `shm_open()` + `mmap()`

### **On Native Windows (`#if defined(_WIN32)`)**
The memory block gracefully falls back to `<windows.h>`:
* **Instantiation:** `CreateFileMappingA()` + `MapViewOfFile()`

### **Key Hashing:**
A random shared-memory name can collide with protected paths on certain UNIX systems or Windows filesystems. To bypass this, the socket handshake room ID is converted strictly into an 8-character hashed prefix (`WTT_<hash>`). This avoids illegal slash character errors (`/tmp/...`) in cross-platform named allocations.

## 4. Temporary File Handling (Avatars)
Previously, the avatar system hardcoded `/tmp/ttt_avatar.png`, which inherently fails on Native Windows (`C:\tmp\` does not exist). The code now correctly invokes Qt’s `QDir::tempPath()` inside `GameLauncher::launchTicTacToe`:

```cpp
// Works on both WSL (/tmp/) and Windows (%TEMP%)
QString avatarPath = QDir::tempPath() + QString("/ttt_avatar_%1.png").arg(opponent);
```

## Summary
By enforcing relative dependency resolution, conditionally compiling Windows/Linux IPC calls, and wrapping file I/O operations inside `std::filesystem`/`QDir`, WizzMania provides a truly ubiquitous "clone and build" experience. Collaborators on WSL can simply configure `CMake`, type `ninja`, and the launcher will seamlessly resolve and open all IPC games without throwing `"Executable Not Found"` errors.
