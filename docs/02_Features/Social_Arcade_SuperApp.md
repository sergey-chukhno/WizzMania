# WizzMania Social Arcade & Super App Ecosystem — Feature Specification 🎮🌐

**Pillar Alignment**: Pillar 4 (The Shell) + Pillar 5 (The Super App Ecosystem)
**Status**: Shell Delivered — SDK & Feature Integrations Pending Implementation
**Last Updated**: 2026-05-09 (Decisions locked after architecture review)

---

## Overview

The **Social Arcade** is WizzMania's answer to WeChat's Mini Programs — a curated, sandboxed platform where social experiences (Games, Music, Video, Community) are hosted within the WizzMania shell. It transforms the app from a messenger into a **social operating system**.

The Arcade operates on two levels:
1. **The Host Environment (Shell)**: The glassmorphic tile grid implemented in `SocialArcade`. **Delivered.**
2. **The Mini-App SDK (Platform)**: The runtime and registry that tiles launch into. **Planned.**

---

## Part 1: Current State (Delivered)

### 1.1 SocialArcade Component
- Collapsible tile grid with animated expand/collapse toggle.
- Fixed expanded height of `340px` in the Messenger Hybrid Scroll container.
- Glassmorphic card tiles with hover effects.
- `categorySelected(QString)` signal emitted on tile click.
- All tile handlers in `MainWindow` are currently stubs.

### 1.2 Layout Evolution
**Decision**: As more integrations are added, the tile grid evolves to a **categorized list/tab view**. The current 2-column grid is appropriate for ≤6 tiles; a tab-based layout is implemented when the registry exceeds 6 registered Mini-Apps.

---

## Part 2: The Mini-App SDK

### 2.1 Design Philosophy

The Arcade must **not** be a hardcoded list of features. Every integration registers itself through a **manifest-based Mini-App Registry** fetched from the server on startup. This ensures:
- **Open/Closed Principle**: New features are added without modifying `SocialArcade`.
- **Server Curated**: The registry can be updated server-side without a client release.
- **Sandbox Safety**: Mini-Apps are isolated from the core Shell.

### 2.2 Mini-App Registry: Remote (Server-Fetched)

**Decision**: The registry is fetched from the server on each session start.

**Flow**:
1. On login, client sends `ARCADE_REGISTRY_REQUEST`.
2. Server returns a JSON/binary manifest of registered Mini-Apps.
3. `SocialArcade` renders tiles from the manifest, not hardcoded entries.
4. New tiles appear to all clients without a client update.

**Manifest structure (concept)**:
```cpp
struct MiniAppManifest {
    QString id;           // "com.wizzmania.tilettwister"
    QString title;        // "TileTwister"
    QString iconPath;     // Server-hosted asset URL or bundled resource
    QString category;     // "Games" | "Music" | "Video" | "Community"
    LaunchMode launchMode;// Process | InShell | WebView
    QString launchTarget; // Executable path or widget class name
};
```

### 2.3 Launch Modes

| Mode | Description | Example |
|---|---|---|
| `Process` | Launches sandboxed child process; communicates via Native Shared Memory IPC | TicTacToe, TileTwister, BrickBreaker |
| `InShell` | Loads a `QWidget` subclass into a dedicated panel within `MainWindow` | Music Player, Whiteboard |
| `WebView` | Embeds `QWebEngineView` for web-based integrations | YouTube sync, Spotify |

### 2.4 IPC Contract (Process Mode)
Games use the existing Native Shared Memory IPC (`GameIPC.h`). This is formalized as the standard SDK contract:
- Fixed-layout shared memory block with magic number header.
- Fields: `game_state`, `player_action`, `opponent_id`, `session_token`.
- Shell monitors IPC blocks via a dedicated `IpcWatcher` thread.
- Game signals termination by writing a sentinel value to shared memory.

---

## Part 3: Social Feature Integrations

### 3.1 Games

**Launch Mode**: `Process`

**Features**:
- Tile displays live activity: "🎮 Alice & Bob are playing TileTwister".
- Click tile → matchmaking modal: "Play Solo" or "Challenge [friend]".
- Score and results posted to the conversation on game end.

**Delivered Games**: TicTacToe, TileTwister, BrickBreaker (IPC already implemented via `GameIPC.h`).

---

### 3.2 Music (Collaborative Listening)

**Launch Mode**: `InShell`

**Concept**: Synchronized listening room — all participants hear the same stream simultaneously.

**Decision**: Support streaming URLs (YouTube, Spotify deep links). No local file requirement.

**Features**:
- Host creates a Listening Room in the Arcade.
- Host provides a streaming URL (YouTube, Spotify, SoundCloud).
- The Shell embeds `QWebEngineView` to resolve and play the stream.
- The Server acts as **sync conductor**: broadcasting `MUSIC_SYNC` events (play, pause, seek + server timestamp) to all room members.
- **Latency compensation**: All clients buffer 2 seconds and synchronize to a server-issued start timestamp. Playback begins at `startTime + buffer` across all clients.
- Chat overlay: Members react and comment in real time during listening.

**Technical notes**:
- Sync protocol: `MUSIC_SYNC_CREATE`, `MUSIC_SYNC_PLAY`, `MUSIC_SYNC_PAUSE`, `MUSIC_SYNC_SEEK` opcodes.
- Platform streaming is handled by the embedded web view — WizzMania does not transcode or retransmit audio.

---

### 3.3 Video (Synchronized Watch Party)

**Launch Mode**: `InShell` (using `QWebEngineView` for streaming URLs)

**Decision**: Support streaming URLs (YouTube, Spotify deep links, etc.).

**Features**:
- Host creates a Watch Party and provides a streaming URL.
- Server broadcasts playback events (play, pause, seek + timestamp) to all connected participants.
- All clients synchronize to the host's playback position on each event.
- Reaction bar: Emoji reactions float over the video in real time.
- Chat sidebar available during watch.

**Sync model**: Identical to Music — server-coordinated timestamp, 2s client buffer.

---

### 3.4 Community Tools

**Launch Mode**: `InShell`

**Phase 1 Features (launch):**

**Polls**
- Create binary or multiple-choice polls visible to all friends or a specific group.
- Live vote count updates in real time via `POLL_VOTE_UPDATE` broadcasts.
- **Poll results stored permanently on the server** (indefinite retention).
- Opcodes: `POLL_CREATE`, `POLL_VOTE`, `POLL_RESULT_UPDATE`.

**Shared Whiteboard** *(Phase 1)*
- Collaborative drawing canvas shared between two or more users.
- Actions (draw stroke, erase, clear) transmitted as delta packets — not full canvas state.
- Delta compression: only changed regions sent, not the full bitmap.
- `QGraphicsScene` renders the canvas; strokes are `QGraphicsPathItem` objects.
- Opcodes: `WHITEBOARD_STROKE`, `WHITEBOARD_ERASE`, `WHITEBOARD_CLEAR`.

**Events**
- Create an event (title, date, description) and invite friends or a group.
- RSVP (Yes/Maybe/No) tracked server-side.
- Event delivered as a special message type in the group or DM conversation.
- Opcodes: `EVENT_CREATE`, `EVENT_RSVP`, `EVENT_UPDATE`.

---

### 3.5 Channel Directory Integration

The Community tile surfaces the **Channel Directory** (defined in `Groups_and_Channels.md`):
- Trending public channels displayed as cards.
- Search by keyword, topic, language.
- One-tap subscribe with immediate content delivery.

---

## Part 4: Live Activity Tiles

Arcade tiles surface real-time social signals from the presence infrastructure:

| Tile | Live Activity Example |
|---|---|
| Games | "🎮 Alice & Bob are playing TileTwister" |
| Music | "🎵 3 friends in a Listening Room" |
| Video | "🎬 Watch Party started by Charlie" |
| Community | "📊 New poll: 12 votes so far" |

**Implementation**: Server maintains an `ArcadeActivity` map updated by presence events. Clients receive delta updates via `ARCADE_ACTIVITY_UPDATE` on session start and on change.

---

## 5. Architectural Decisions Log

| Decision | Choice | Rationale |
|---|---|---|
| Mini-App Registry | Remote, server-fetched | Server can add tiles without client update |
| Music/Video Sync | Streaming URLs (YouTube, Spotify) | No local file requirement; broader use cases |
| Whiteboard | Phase 1 feature | High engagement value; delta packets keep it manageable |
| Poll Results | Stored permanently on server | Auditability and result persistence |
| Arcade Layout | Categorized tab view when > 6 tiles | Scales gracefully as integrations grow |
| Launch trigger | Stub handlers in `MainWindow` | Connected to SDK in next implementation phase |
