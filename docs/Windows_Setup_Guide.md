# WizzMania: Windows Setup Guide & Building from Scratch

Welcome to the WizzMania repository! This guide will walk you through compiling and running the complete WizzMania Client, Server, and integrated games (TileTwister, Cyberpunk Cannon Shooter) completely from scratch on Windows.

We have heavily optimized the codebase for portability. You do **not** need to install SQLite, SDL2, or other generic C++ libraries manually—the CMake build system will automatically fetch those source amalgamation files from the internet and compile them for you natively!

However, there are two mandatory major frameworks you need to install on your machine: Qt6 and SFML.

---

## 🛠️ Prerequisites

### 1. Visual Studio & C++ Build Tools
You must have the MSVC (Microsoft Visual C++) compiler installed. 
1. Download **Visual Studio Community 2022** (or higher).
2. Run the Visual Studio Installer.
3. Check the box for **"Desktop development with C++"**. 
4. Ensure the **"C++ CMake tools for Windows"** component is checked in the right panel.

### 2. Qt 6 Framework
The Chat Client relies on the Qt6 framework for its GUI toolkit and cross-platform networking abstractions.
1. Download the **Qt Online Installer** from the official Qt website (Open Source version).
2. During installation, select **Qt 6.x.x** (the latest version).
3. Under the Qt 6.x dropdown, make sure to check the box for **MSVC 2019 64-bit** (or MSVC 2022 if explicitly available). You do *not* need MinGW, Android, or iOS.
4. Note your installation path! (Usually `C:\Qt`).

### 3. SFML (Simple and Fast Multimedia Library)
The integrated game `BrickBreaker`/`CyberpunkCannonShooter` requires SFML to be present on the system.
The easiest way to install this on Windows is using Microsoft's package manager `vcpkg` or by downloading pre-compiled binaries from the SFML website.

**Option A (Vcpkg - Recommended):**
```cmd
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install sfml:x64-windows
```

**Option B (Manual Download):**
1. Download the **Visual C++** binaries (matching your VS version) from [sfml-dev.org](https://www.sfml-dev.org/download.php).
2. Extract them to a static path, such as `C:\Libraries\SFML`.

---

## 🚀 Building the Project

Open **Developer Command Prompt for VS 2022** (or standard Windows Terminal/CMD).

### Step 1: Clone the Repository
```cmd
git clone https://github.com/your-repo/WizzMania.git
cd WizzMania
```

### Step 2: Configure CMake
You must tell CMake where to find Qt6 and SFML using the `CMAKE_PREFIX_PATH` flag.
*Replace the paths below with your actual Qt and SFML/vcpkg installation paths.*

```cmd
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64;C:\vcpkg\installed\x64-windows"
```
*(Note: CMake will now reach out to the internet to fetch and configure SQLite and SDL2 autonomously during this step. Ensure you have an internet connection!)*

### Step 3: Compile
Once configured, compile the entire suite (Server, Client, and all Games) using the Release configuration:

```cmd
cmake --build build --config Release
```

---

## 🎮 Running WizzMania Locally

We have provided a Windows orchestration batch script to instantly spawn the Server alongside two separate UI Clients so you can test chatting between "two machines" instantly!

To test the multi-client network:
1. First, you must start the Server. Open a Command Prompt, navigate to `build\server`, and run `wizz_server.exe`. It will bind to Port 8080 and initialize the SQLite database automatically.
2. In the root of the repository, double-click the file named **`launch_multiclient.bat`**.
3. Two separate WizzMania Chat windows (`wizz_client`) will pop up shortly after.
4. You can click on the game icons inside the chat UI. Advanced dynamic path discovery guarantees the games will correctly launch themselves out of the `build/games/.../Release/` outputs.

Happy developing!
