# Tile Twister

A modern, polished C++ implementation of the 2048 game using SDL2, featuring a robust architecture, persistence, achievements, and procedural audio.

## 🚀 Features
*   **Core 2048 Logic**: Smooth sliding mechanics, merging rules, and win/loss states.
*   **Persistence**: Automatically saves your game state and high scores to disk.
*   **Leaderboard**: Tracks top 5 scores with persistence.
*   **Achievements**: Unlockable visual milestones (Medal, Cups) with "Glass" popup notifications.
*   **Visuals**: Dark/Light modes, glassmorphism UI, and procedural animations.

## 📂 Project Structure
*   `src/core`: Pure C++ game logic (Platform independent).
*   `src/engine`: SDL2 wrappers (Graphics, Window, Sound).
*   `src/game`: Main application loop, Input, and UI.
*   `tests/`: Comprehensive GoogleTest suite (Unit & Integration).
*   `docs/`: Detailed design and coverage documentation.

## 🛠️ How to Build
This project uses **CMake**, ensuring compatibility with Windows, macOS, and Linux.

### Quick Start (macOS/Linux)
```bash
# 1. Generate build
cmake -S . -B build

# 2. Build project
cmake --build build

# 3. Run Game
./build/TileTwister
```

### Windows (Visual Studio)
1.  Open folder in Visual Studio 2022.
2.  Let CMake configure automatically.
3.  Select `TileTwister.exe` and press **F5**.

## 🧪 Testing
We maintain a high standard of quality with automated testing.

### Running Tests
After building, you can run the test suites:

**Option A: CTest (Recommended)**
```bash
cd build
ctest --verbose
```

**Option B: Direct Executables**
*   **Unit Tests**: `./build/TileTwister_Tests`
*   **Integration Tests**: `./build/IntegrationTests`

### Test Coverage & Scenarios
*   **Coverage Report**: See [docs/TestCoverage.md](docs/TestCoverage.md) for a detailed breakdown of covered features (Core Logic: 100%, Persistence: 100%).
*   **Integration Scenarios**: See [tests/integration/TestScenarios.md](tests/integration/TestScenarios.md) for the actual test plans used.

## 📄 License
This project is for educational and portfolio purposes.
