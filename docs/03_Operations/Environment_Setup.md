# WizzMania Environment Setup & Portability 💻⚙️

WizzMania is designed for seamless portability across **macOS**, **Windows**, and **Linux (WSL)**. We achieve this by using a "Zero-Install" dependency model and platform-agnostic C++ code.

---

## 1. Quick Start (Build Commands)

Regardless of your OS, the build process is standardized via CMake:

```bash
# 1. Configure the project
cmake -B build -S .

# 2. Compile all targets
# (Includes Server, Client, Games, and Tests)
cmake --build build -j$(nproc)
```

---

## 2. Platform-Specific Guides

### 2.1 macOS (Native)
- **Requirement**: Xcode Command Line Tools.
- **Dependency Management**: CMake automatically clones required SDKs (SFML, SDL2) into `build/_deps/`.

### 2.2 Windows (Native MSVC / MinGW)
- **Requirement**: Visual Studio 2022 (with Desktop C++) or MinGW-w64.
- **Qt Path**: Ensure `Qt6_DIR` is set in your environment variables.
- **Portability**: The client automatically detects `.exe` extensions and Windows-specific temporary paths (`%TEMP%`).

### 2.3 Linux / WSL (Ubuntu)
- **Requirement**: `build-essential`, `cmake`, `ninja-build`, `libx11-dev`.
- **Display**: For GUI support in WSL, ensure **WSLg** (Windows 11) is active or an X-Server (VcXsrv) is configured.

---

## 3. Portability Architecture

### 3.1 FetchContent (Zero-Install)
We avoid the "missing library" nightmare by having CMake clone precisely the versions we need during the configuration phase:
- **SFML 3.0**: Fetched directly for game rendering.
- **SDL2 Ecosystem**: Cloned for TileTwister integration.
- **GoogleTest**: Integrated for automated verification.

### 3.2 Dynamic Path Resolution
The `GameLauncher.h` does not use hardcoded paths. It identifies the application directory at runtime and searches for game binaries in relative locations:
- `build/games/`
- `build/bin/`
- `build/bin/Debug/` (Windows specific)

### 3.3 Abstracted IPC
Our Shared Memory implementation (`NativeSharedMemory<T>`) uses conditional compilation to bridge the gap between POSIX and Win32:
- **POSIX**: `shm_open` + `mmap`.
- **Win32**: `CreateFileMappingA` + `MapViewOfFile`.

---

## 4. Troubleshooting
- **CMake fails to find Qt**: Provide the path explicitly: `cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64"`
- **Game fails to launch**: Ensure you built the `games` target (or use `cmake --build build`).
- **Shared Memory Error**: On Linux, ensure you have permissions for `/dev/shm`.
