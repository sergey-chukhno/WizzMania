# WizzMania - WSL Onboarding & Build Guide

This document provides definitive instructions for building and running the WizzMania ecosystem on **Windows Subsystem for Linux (WSL)**. By using WSL, you benefit from a native Linux environment (POSIX) while keeping your Windows workflow.

---

## 1. Prerequisites (WSLg)

Ensure you have **WSL2** installed. Modern WSL versions (Windows 10 Build 19044+ or Windows 11) support **WSLg**, which automatically handles GUI applications (Qt6/SFML) and audio.

**Update WSL in PowerShell (Admin):**
```powershell
wsl --update
wsl --shutdown
```

---

## 2. Environment Setup & Cloning

> [!IMPORTANT]
> **Performance Tip**: Always clone the repository inside the WSL Linux filesystem (e.g., `~/projects/`).
> **Avoid** `/mnt/c/` or any Windows-mapped drives, as the translation layer makes CMake and Ninja extremely slow.

Open your **Ubuntu** terminal:
```bash
mkdir -p ~/dev && cd ~/dev
git clone https://github.com/sergey-chukhno/WizzMania.git
cd WizzMania
```

---

## 3. Install Comprehensive Dependencies

WizzMania requires several system libraries for the Server (SSL/SQLite), Client (Qt6), and Games (SFML/SDL2). Run this single command to install everything:

```bash
sudo apt update && sudo apt install -y \
    build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-multimedia-dev qt6-multimedia-all \
    libssl-dev libsqlite3-dev \
    libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-image-dev \
    libsfml-dev libxrandr-dev libxcursor-dev libudev-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
```

> [!NOTE]
> WizzMania uses **SFML 3.0** for modern game features. Since Ubuntu repositories often only provide version 2.6, CMake will automatically download and build SFML 3.0 for you during the first compilation. This is why the extra `libx11` and `libgl` libraries above are required.

---

## 4. The Unified Build Pipeline

Building the entire project (Server, Client, and all Games) is handled by the root `CMakeLists.txt`.

```bash
# From the WizzMania root directory
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja
```

---

## 5. Execution Flow

To verify the full stack, you need to run the server and at least two clients.

### A. Start the Server
```bash
./server/wizz_server
```

### B. Launch Clients (Multi-Client Test)
WizzMania includes a launch script to quickly open two client windows for testing:
```bash
# In a new terminal tab
cd ~/dev/WizzMania
chmod +x launch_multiclient.sh
./launch_multiclient.sh
```

---

## 6. Running the Test Suite

Verify your environment by running the GoogleTest suite:
```bash
cd build
ctest --output-on-failure
# Or run specific tests
./tests/unit_tests
```

---

## 7. Troubleshooting & Common Pitfalls

### Graphics Issues
If the Qt window doesn't appear:
- Ensure you aren't using a VPN that blocks local X11 forwarding.
- Try: `export DISPLAY=:0` or `export DISPLAY=$(grep -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0.0`.

### QSharedMemory / IPC Errors
If TileTwister or TicTacToe fails to track scores:
- **Permission Denied**: On some WSL setups, `/dev/shm` might have restricted permissions.
- Fix: `sudo chmod 777 /dev/shm`.

### Audio in SFML
If you get "No audio device found":
- Ensure PulseAudio is running on WSL (usually automatic with WSLg).
- Run `sudo apt install pmaports-dev` (if available) or `libpulse-dev`.
