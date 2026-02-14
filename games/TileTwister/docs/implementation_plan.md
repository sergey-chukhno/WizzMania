# Implementation Plan - Tile Twister

# Goal
Build a C++2048 clone using modern C++17/20 and SDL2, focusing on clean architecture and testability.

## User Review Required
> [!NOTE]
> **Portability strategy**: We will use CMake + a package manager (CMake's `FetchContent`) to handle SDL2 and GTest. This ensures your Windows teammates can just run `camke` and it works, without needing Docker containers which struggle with GUI apps.

## Proposed Milestones

### Milestone 1: Project Skeleton & Build System
**Goal**: A compiling "Hello World" that links GTest and SDL2.
- Setup `CMakeLists.txt`
- Configure `GoogleTest`
- Configure `SDL2`
- Verify compiling on Mac (User) and ensure CMake scripts## Phase E: Architecture Refactoring & Cleanup
Currently, `Game` class does too much (God Object). We need to split it.
### 1. Input Management
*   Extract `InputManager` class.
*   Maps raw SDL scancodes (e.g., `SDLK_UP`) to logical Game Actions (`Action::MoveUp`).
*   **Why?** Allows remapping keys or adding Controller support later easily.

### 2. State Management
*   The game currently just "runs". It needs states: `Menu`, `Playing`, `GameOver`.
*   Implement a simple `GameState` enum or State Pattern.

## Detailed Design: Phase E (Refactoring)

### 1. InputManager (The Translator)
**Problem**: `Game.cpp` currently has raw SDL event loops and `SDLK_UP` hardcoded.
**Solution**: Create an abstract "Action" layer.
*   **Enum**: `enum class Action { None, Up, Down, Left, Right, Quit, Restart, Confirm };`
*   **Class**: `InputManager`
    *   `Action pollAction()`: Polls SDL events and returns a high-level `Action`.
    *   **Benefit**: If we want to add WASD support, we just change `InputManager`. The `Game` class doesn't care.

### 2. Game State (The Flow)
**Problem**: The game is always "Playing". We need menus and game over screens.
**Solution**: Finite State Machine (FSM).
*   **Enum**: `enum class GameState { MainMenu, Playing, GameOver };`
*   **Enum**: `enum class GameState { MainMenu, Playing, GameOver, Options, Leaderboard, Achievements };`
*   **Loop**:
    ```cpp
    void Game::update() {
        switch (m_state) {
            case GameState::MainMenu: updateMenu(); break;
            case GameState::Playing:  updateGame(); break;
            case GameState::GameOver: updateGameOver(); break;
            case GameState::Options:  updateOptions(); break;
            // ...
        }
    }
    ```

### 3. Menu Structure (The Requirements)
*   **MainMenu**:
    *   **Start Game**: New Game.
    *   **Load Game**: Resume saved session (Future).
    *   **Options**: Audio, Theme, Controls.
    *   **Leaderboard**: Top scores.
    *   **Achievements**: Unlocks.
    *   **Quit**: Exit App.

## Phase F: Complete Gameplay Loop
### 1. Scoring System
*   **Core**: Update `GameLogic::slideAndMergeRow` to return points earned.
*   **Game**: Track `currentScore` and `bestScore`.
*   **UI**: Render Score at the top.

### 2. Game Over State
*   **Logic**: Implement `GameLogic::isGameOver(grid)` (checking full grid + no possible moves).
*   **UI**: Detect Game Over -> Stop input -> Show "Game Over" text -> Press 'R' to restart.

## Phase I: Visual Overhaul (Design & Polish)
### 1. Rendering Architecture Update (The "Pro" Look)
*   **Current State**: `SDL_RenderFillRect` (Square, sharp, basic).
*   **New Approach**: **Texture Modulation**.
    *   We will load a single asset `assets/tile_rounded.png` (white, transparent corners).
    *   We use `SDL_SetTextureColorMod` to tint this sprite dynamically for each tile value.
    *   *Result*: Smooth, anti-aliased rounded corners identical to modern apps.

### 2. Layout System
*   **Window**: 600x800.
*   **Header (Top 200px)**:
    *   **Title**: "2048" (Left aligned or Centered).
    *   **ScoreBox**: Rounded box showing current Score.
    *   **BestBox**: Rounded box showing All-time Best.
*   **Grid Container (Bottom 600x600)**:
    *   Dynamic sizing based on padding.
    *   Background container color (Board Color).

### 3. Color Palette (Reference Matching)
*   **Light Theme**:
    *   BG: `#faf8ef`
    *   Grid: `#bbada0`
    *   Empty Tile: `#cdc1b4`
    *   Text: Dark Grey (`#776e65`).
*   **Dark Theme**:
    *   BG: `#333333`
    *   Grid: `#4d4d4d`
    *   Empty Tile: `#595959`
    *   Text: White (`#f9f6f2`).

### 4. Game Over Screen
*   **Overlay**: Blur or Darken the grid.
*   **Content**:
    *   "Game Over!"
    *   "Your Score: 12345"
    *   "Best: 99999"
    *   "Try Again" Button.

## Phase G: Integration Testing
We have Unit Tests (GTest) for `Core`. We need Integration Tests for `Game`.
*   **Headless Game Loop**: Create a `HeadlessGame` that runs `GameLogic` without `Renderer`.
*   **Scripted Scenarios**: Feed a list of inputs (Up, Up, Left...) and verify the final Grid state matches expected output.
*   **Why?** Ensures the "Controller" (Game class) correctly wires inputs to logic.

## Phase J: Animations (Game Feel)
*   **Objective**: Implement smooth sliding transitions and tile spawning/merging effects.
*   **Technical**:
    *   Transition from "Instant State" rendering to "Interpolated State".
    *   Update `GameLogic` to return `MoveEvents` (Start, End positions).
    *   Create `AnimationManager` to handle tweens (Linear/EaseOut).
    *   **Why?**: This is significantly more complex than static rendering but essential for the "Premium" feel.

## Phase K: Audio (Sound System)
*   **Objective**: Add Sound Effects (Slide, Merge, Game Over) and Music.
*   **Technical**:
    *   Integrate `SDL_mixer`.
    *   Load WAV/MP3 assets.
    *   Trigger sounds on Event (Move, Merge).

## Phase L: Persistence & Leaderboard
*   **Objective**: Save/Load game state and track top scores with nicknames.
*   **Technical**:
    *   **PersistenceManager**: Handles all File I/O (text-based format).
    *   **Save/Load**: Serialize `Grid` and `Score` on exit/startup.
    *   **Leaderboard**: Track Top 5 scores (`struct ScoreEntry { name, score }`).
    *   **Nickname Input**: If High Score achieved, show "Enter Name" UI state.
    *   **Architecture**: `src/game/PersistenceManager` (Bridge between Game and Filesystem).

## Verification Plan
### Automated Tests
```bash
ctest --verbose
```
### Manual Verification
- Verify the build works on your Mac.
- Visual check of the game window and movement animations.
