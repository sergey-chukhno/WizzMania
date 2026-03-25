# Slide 9: Matchmaking & TicTacToe Multijoueur

## 1. Feature Overview
This slide demonstrates the multiplayer "Invite and Play" flow. It shows how the UI, the network, and the game engine (SFML) coordinate to start a 1v1 match.

## 2. How it Works
1. **Discovery**: User sees a "Controller" icon next to an online friend.
2. **Invite**: Clicking the icon sends a `GameInvite` packet via TLS.
3. **Handshake**: The opponent accepts; the server notifies both.
4. **Launch**: `GameBridge` saves the opponent's avatar to a temp file and launches the `TicTacToe` process with command-line arguments.
5. **Sync**: The game then connects to the IPC memory to exchange moves.

## 3. Why This Code (Rationale)
- **`QProcess`**: We use `QProcess` to launch games as external logic. This sandbox approach means a crash in TicTacToe won't crash the main WizzMania chat.
- **Command-Line Arguments**: Passing the avatar path (`--avatarPath`) and username via CLI is a robust way to "Inject" state into a fresh process.
- **`QDir::tempPath()`**: Hardcoding paths `/tmp/` is bad practice. `tempPath()` ensures portability (Windows/WSL/Linux).

## 4. How the Code Implements This
### `MainWindow::populateContactList()`
Dynamically creates a `QPushButton` for each online friend. The lambda `[this, name = contact.username]` captures the friend's name for the invite.
### `GameBridge::startTicTacToe()`
This is the **Orchestrator**. It saves the `QPixmap` (avatar) to disk because SFML (the game engine) cannot read Qt memory assets directly.
### `PlayingState::update()` (Game Side)
The game locks the memory, reads the `board[9]` array (representing the 3x3 grid), and updates the visual icons (Circle/Cross).

## 5. Strategic Analysis

### Advantages
- **UX**: Seamless transition from chat to game.
- **Asset Sharing**: Reusing the avatar from the server inside the local game engine makes the experience feel integrated.

### Drawbacks / Limits
- **File Rights**: Writing to `tempPath` requires write permissions.
- **Cleanup**: We need to delete the temporary avatar files after the game exits to avoid cluttering the user's disk.

### Edge Cases
- **Simultaneous Invites**: If two people invite each other at the exact same time (Race Condition), the server must handle which "Match" wins or join them together.
- **Process Orphans**: If WizzMania is closed while TicTacToe is still running, the game process should ideally exit (detected by the shared memory disappearing).
