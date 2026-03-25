# Engineering Analysis: Game Integration Portability

## 1. The Issue
Currently, `WizzMania` integrates games using **Hardcoded Absolute Paths** to executables on the developer's local machine (e.g., `/Users/sergeychukhno/Desktop/...`).

### Impact on Collaborators
When a collaborator clones the repository:
1.  **Missing Files**: They do not have the game repositories (`TileTwister`, `BrickBreaker`) installed.
2.  **Broken Paths**: Even if they have the games, their file system structure (username, folders) will differ, causing `QProcess::startDetached` to fail.
3.  **Platform incompatibility**: Hardcoded Unix paths (`/Users/...`) break entirely on Windows (`C:\...`).

---

## 2. Proposed Architecture (The "Correct" Way)

To solve this, we must treat games as **Embedded Dependencies** rather than external loose files.

### Step 1: Git Submodules
We should include the game repositories as submodules within `WizzMania`.
```bash
git submodule add https://github.com/user/TileTwister.git games/TileTwister
git submodule add https://github.com/user/BrickBreaker.git games/BrickBreaker
```

### Step 2: Unified Build System
Update `CMakeLists.txt` to build these games automatically.
```cmake
# Root CMakeLists.txt
add_subdirectory(games/TileTwister)
add_subdirectory(games/BrickBreaker)
```
This ensures that when a collaborator runs `cmake --build .`, **everything** is built.

### Step 3: Relative Paths
The code should locate games *relative* to the `WizzMania` executable, not an absolute path.

**Implementation Logic:**
1.  Get the running application's directory: `QCoreApplication::applicationDirPath()`.
2.  Traverse to the known relative build output of the games.

```cpp
QString appDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    // On macOS, bundle structure is more complex
    QString gamePath = appDir + "/../../../games/TileTwister/TileTwister.app/Contents/MacOS/TileTwister";
#else
    QString gamePath = appDir + "/../games/TileTwister/TileTwister.exe";
#endif
```

---

## 3. Alternative: Configuration File (The "Flexible" Way)
If bundling games is too complex or they are closed-source, use a `config.json` file.
1.  The app reads `config.json` on startup.
2.  Collaborators create their own `config.json` specifying where *their* games are installed.

**Recommendation:** **Method 2 (Bundling/Submodules)** is best for a cohesive open-source project. **Method 3** is best if games are independent commercial products.
