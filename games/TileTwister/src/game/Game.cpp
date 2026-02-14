
#include "Game.hpp"
#include "PersistenceManager.hpp"
#include <SDL.h>
#include <cmath>
#include <iostream>
#include <string>

namespace Game {

Game::Game()
    : m_context(), m_window("Tile Twister - 2048", WINDOW_WIDTH, WINDOW_HEIGHT),
      m_renderer(m_window, WINDOW_WIDTH, WINDOW_HEIGHT),
      m_font("assets/ClearSans-Bold.ttf", 40),      // Tile Font
      m_fontTitle("assets/ClearSans-Bold.ttf", 80), // Title
      m_fontSmall("assets/ClearSans-Bold.ttf", 16), // Labels
      m_fontTiny("assets/ClearSans-Bold.ttf",
                 14), // Compact Labels (Smaller to fit)
      m_fontMedium("assets/ClearSans-Bold.ttf", 30), // Score Values
      m_inputManager(), m_grid(), m_logic(), m_isRunning(true),
      m_state(GameState::MainMenu), m_previousState(GameState::MainMenu),
      m_menuSelection(0), m_darkSkin(false), m_soundOn(true), m_score(0),
      m_bestScore(0), m_showAchievementPopup(false),
      m_popupAchievementIndex(-1), m_popupTimer(0.0f) {

  // Load Unlocked Achievements
  m_unlockedAchievements = PersistenceManager::loadAchievements();

  // Load Best Score
  auto scores = PersistenceManager::loadLeaderboard();
  if (!scores.empty()) {
    m_bestScore = scores[0].score;
  }

  // Load Assets
  try {
    m_tileTexture = std::make_unique<Engine::Texture>(
        m_renderer, "assets/tile_rounded.png");
  } catch (const std::exception &e) {
    // Fallback or error logging
    SDL_Log("Failed to load tile texture: %s", e.what());
  }

  // Initial Setup
  if (m_soundManager.init()) {
    m_soundManager.loadSound("move", "assets/move.wav");
    m_soundManager.loadSound("merge", "assets/merge.wav");
    m_soundManager.loadSound("spawn", "assets/spawn.wav");
    m_soundManager.loadSound("invalid", "assets/invalid.wav");
    m_soundManager.loadSound("gameover", "assets/gameover.wav");
    m_soundManager.loadSound("score", "assets/score.wav");
    m_soundManager.loadSound("fireworks", "assets/fireworks.wav");
  }

  // Try load Button BG
  try {
    m_buttonTexture =
        std::make_unique<Engine::Texture>(m_renderer, "assets/button_bg.png");
  } catch (...) {
    m_buttonTexture = nullptr;
  }

  // Try load Star
  try {
    m_starTexture =
        std::make_unique<Engine::Texture>(m_renderer, "assets/star.png");
  } catch (...) {
    m_starTexture = nullptr;
  }

  // Phase Q: Load Grid Menu Assets
  try {
    m_glassTileTexture =
        std::make_unique<Engine::Texture>(m_renderer, "assets/tile_glass.png");
    // Phase R: Use Additive Blending for Glass (Black BG becomes transparent)
    if (m_glassTileTexture) {
      m_glassTileTexture->setBlendMode(SDL_BLENDMODE_ADD);
    }
  } catch (...) {
    m_glassTileTexture = nullptr;
  }

  try {
    m_iconsTexture =
        std::make_unique<Engine::Texture>(m_renderer, "assets/menu_icons.png");
  } catch (...) {
    m_iconsTexture = nullptr;
  }

  // Load Logo with Fuzzy Color Key
  // Removes White (255,255,255) and Light Grey (down to ~200) to clear
  // checkerboard
  try {
    m_logoTexture = std::make_unique<Engine::Texture>(
        m_renderer, "assets/logo.png", 255, 255, 255,
        60); // Threshold 60 catches light greys
  } catch (const std::exception &e) {
    SDL_Log("Failed to load logo: %s", e.what());
  }

  // Load Achievement Icons
  m_achievementTextures.clear();
  const char *achievementFiles[] = {"assets/medal.png", "assets/cup.png",
                                    "assets/super_cup.png"};
  for (const char *file : achievementFiles) {
    try {
      m_achievementTextures.push_back(
          std::make_unique<Engine::Texture>(m_renderer, file));
    } catch (...) {
      m_achievementTextures.push_back(nullptr);
    }
  }

  resetGame();
}

// ... (methods) ...

// Helper for Procedural Icons (ADVANCED)
// Helper for Procedural Icons (ADVANCED)
void drawProceduralIcon(SDL_Renderer *renderer, int type, int x, int y,
                        int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  SDL_Color c = {r, g, b, a};

  switch (type) {
  case 0: // Start (Play Triangle)
  {
    int pad = size / 4;
    int shiftX = size / 16;

    SDL_Vertex v[3];
    v[0].position = {(float)x + pad + shiftX, (float)y + pad};
    v[0].color = c;
    v[1].position = {(float)x + pad + shiftX, (float)y + size - pad};
    v[1].color = c;
    v[2].position = {(float)x + size - pad + shiftX, (float)y + size / 2};
    v[2].color = c;
    SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
    break;
  }
  case 1: // Load (Folder)
  {
    SDL_Rect body = {x + size / 4, y + size / 3, size / 2, size / 2 - size / 8};
    SDL_RenderFillRect(renderer, &body);
    SDL_Rect tab = {x + size / 4, y + size / 4, size / 4, size / 8};
    SDL_RenderFillRect(renderer, &tab);
    break;
  }
  case 2: // Options (Hamburger)
  {
    int h = size / 10;
    int w = size / 2;
    int startX = x + size / 4;
    int gap = size / 6;

    SDL_Rect r1 = {startX, y + size / 4 + 4, w, h};
    SDL_Rect r2 = {startX, y + size / 4 + 4 + gap, w, h};
    SDL_Rect r3 = {startX, y + size / 4 + 4 + gap * 2, w, h};
    SDL_RenderFillRect(renderer, &r1);
    SDL_RenderFillRect(renderer, &r2);
    SDL_RenderFillRect(renderer, &r3);
    break;
  }
  case 3: // Leaderboard (Cup)
  {
    int cupW = size / 2;
    int cupH = size / 4;
    int cx = x + (size - cupW) / 2;

    SDL_Rect top = {cx, y + size / 4, cupW, cupH};
    SDL_RenderFillRect(renderer, &top);

    SDL_Vertex v[3];
    v[0].position = {(float)cx, (float)y + size / 4 + cupH};
    v[0].color = c;
    v[1].position = {(float)cx + cupW, (float)y + size / 4 + cupH};
    v[1].color = c;
    v[2].position = {(float)x + size / 2,
                     (float)y + size / 4 + cupH + size / 8};
    v[2].color = c;
    SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);

    SDL_Rect stand = {x + size / 2 - size / 8, y + 3 * size / 4, size / 4,
                      size / 16};
    SDL_RenderFillRect(renderer, &stand);
    break;
  }
  case 4: // Achievements (Diamond)
  {
    SDL_Vertex v[4];
    float cx = x + size / 2;
    float cy = y + size / 2;
    float halfW = size / 3;
    float halfH = size / 3;

    SDL_Vertex t[3];
    t[0].position = {cx, cy - halfH};
    t[0].color = c;
    t[1].position = {cx + halfW, cy};
    t[1].color = c;
    t[2].position = {cx - halfW, cy};
    t[2].color = c;
    SDL_RenderGeometry(renderer, nullptr, t, 3, nullptr, 0);

    SDL_Vertex b[3];
    b[0].position = {cx + halfW, cy};
    b[0].color = c;
    b[1].position = {cx, cy + halfH};
    b[1].color = c;
    b[2].position = {cx - halfW, cy};
    b[2].color = c;
    SDL_RenderGeometry(renderer, nullptr, b, 3, nullptr, 0);
    break;
  }
  case 5: // Quit (Stop)
  {
    SDL_Rect rect = {x + size / 3, y + size / 3, size / 3, size / 3};
    SDL_RenderFillRect(renderer, &rect);
    SDL_RenderDrawLine(renderer, x + size / 2, y + size / 4, x + size / 2,
                       y + size / 3 - 2);
    break;
  }
  case 6: // Back (Left Arrow)
  {
    SDL_Vertex v[3];
    int pad = size / 4;
    v[0].position = {(float)x + pad, (float)y + size / 2}; // Tip (Left)
    v[0].color = c;
    v[1].position = {(float)x + size - pad, (float)y + pad}; // Top Right
    v[1].color = c;
    v[2].position = {(float)x + size - pad, (float)y + size - pad}; // Bot Right
    v[2].color = c;
    SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);

    // Optional: Box/Stick? Clean triangle is usually sufficient for "Back".
    break;
  }
  case 7: // Try Again (Play Triangle)
  {
    // Simple Right-Pointing Triangle
    int pad = size / 4;
    SDL_Vertex v[3];
    // Tip (Right)
    v[0].position = {(float)x + size - pad, (float)y + size / 2};
    v[0].color = c;
    // Top Left
    v[1].position = {(float)x + pad, (float)y + pad};
    v[1].color = c;
    // Bot Left
    v[2].position = {(float)x + pad, (float)y + size - pad};
    v[2].color = c;
    SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
    break;
  }
  case 8: // Menu (Grid 2x2)
  {
    int pad = size / 3;
    int gap = size / 12;
    int sqSize = (size - 2 * pad - gap) / 2;

    // Top Left
    SDL_Rect r1 = {x + pad, y + pad, sqSize, sqSize};
    // Top Right
    SDL_Rect r2 = {x + pad + sqSize + gap, y + pad, sqSize, sqSize};
    // Bot Left
    SDL_Rect r3 = {x + pad, y + pad + sqSize + gap, sqSize, sqSize};
    // Bot Right
    SDL_Rect r4 = {x + pad + sqSize + gap, y + pad + sqSize + gap, sqSize,
                   sqSize};

    SDL_RenderFillRect(renderer, &r1);
    SDL_RenderFillRect(renderer, &r2);
    SDL_RenderFillRect(renderer, &r3);
    SDL_RenderFillRect(renderer, &r4);
    break;
  }
  }
}

// Game::drawGlassButton definition removed (moved to end of file)

void Game::run() {
  std::cout << "Game Loop Started." << std::endl;

  Uint64 lastTime = SDL_GetTicks64();

  while (m_isRunning) {
    Uint64 currentTime = SDL_GetTicks64();
    float dt = (currentTime - lastTime); // ms
    lastTime = currentTime;

    handleInput();
    update(dt);
    render();

    // Cap at ~60 FPS (Optional, vsync is better but simple delay works)
    if (dt < 16) {
      SDL_Delay(16 - dt);
    }
  }

  std::cout << "Game Loop Ended." << std::endl;
}

void Game::handleInput() {
  int mx = 0, my = 0;
  bool clicked = false;
  Action action = m_inputManager.pollAction(mx, my, clicked);

  if (action == Action::Quit) {
    m_isRunning = false;
    return;
  }

  // Specific Handling for Playing State Buttons (Global check simplifies
  // things if state matches)
  if (m_state == GameState::Playing && clicked) {
    // Toolbar Button Detection (Enlarged Hitboxes)
    // Restart: X=20, Y=120, W=130, H=40
    if (mx >= 20 && mx <= 150 && my >= 120 && my <= 160) {
      resetGame();
      return;
    }
    // Options: X=460, Y=120, W=130, H=40
    if (mx >= 460 && mx <= 590 && my >= 120 && my <= 160) {
      m_previousState = GameState::Playing; // Track history
      m_state = GameState::Options;
      return;
    }
  }

  // Regular Action Handling
  // If Animating, block Input (except Quit?)
  if (m_state == GameState::Animating)
    return;

  switch (m_state) {
  case GameState::Animating:
    break; // Handled above, but satisfies switch
  case GameState::MainMenu:
    handleInputMenu(action, mx, my, clicked);
    break;

  case GameState::Playing:
    handleInputPlaying(action, mx, my, clicked);
    break;

  case GameState::Options:
    handleInputOptions(action, mx, my, clicked);
    break;

  case GameState::BestScores:
    handleInputBestScores(action, mx, my, clicked);
    break;

  case GameState::Achievements:
    handleInputAchievements(action, mx, my, clicked);
    break;

  case GameState::LoadGame:
    handleInputPlaceholder(action, mx, my, clicked);
    break;

  case GameState::SavePrompt:
    handleInputSavePrompt(action, mx, my, clicked);
    break;

  case GameState::GameOver:
    handleInputGameOver(action, mx, my, clicked);
    break;
  }
}

// --- INPUT HANDLERS ---

void Game::handleInputMenu(Action action, int mx, int my, bool clicked) {
  // Mouse Hover Logic for 3x2 Grid
  // Layout matches renderMenu (Compact Mode):
  int tileSize = 105;
  int gap = 12;
  int gridW = (tileSize * 3) + (gap * 2);

  // Center X
  int startX = (WINDOW_WIDTH - gridW) / 2;

  // Center Y (Calculated Dynamic - Matches renderMenu)
  int cardH = 400;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;
  int startY = cardY + 235;

  // Check all 6 buttons
  int hoverIndex = -1;
  for (int i = 0; i < 6; ++i) {
    int row = i / 3;
    int col = i % 3;
    int bx = startX + col * (tileSize + gap);
    int by = startY + row * (tileSize + gap);

    if (mx >= bx && mx <= bx + tileSize && my >= by && my <= by + tileSize) {
      hoverIndex = i;
      break;
    }
  }

  if (hoverIndex != -1) {
    m_menuSelection = hoverIndex;
    if (clicked) {
      action = Action::Select;
    }
  }

  if (action == Action::Select) {
    switch (m_menuSelection) {
    case 0: // Start
      m_state = GameState::Playing;
      m_soundManager.playOneShot("start", 64);
      resetGame();
      m_grid.spawnRandomTile(); // Ensure 2 tiles at start
      break;
    case 1: // Load
      if (PersistenceManager::loadGame(m_grid, m_score)) {
        m_state = GameState::Playing;
        m_soundManager.playOneShot("start", 64);
      } else {
        m_soundManager.playOneShot("invalid", 64);
      }
      m_previousState = GameState::MainMenu;
      m_menuSelection = 0;
      break;
    case 2: // Options
      m_state = GameState::Options;
      m_previousState = GameState::MainMenu;
      m_menuSelection = 0;
      break;
    case 3: // Best Scores
      m_state = GameState::BestScores;
      m_previousState = GameState::MainMenu;
      m_menuSelection = 0;
      break;
    case 4: // Achievements
      m_state = GameState::Achievements;
      m_previousState = GameState::MainMenu;
      m_menuSelection = 0;
      break;
    case 5: // Quit
      m_isRunning = false;
      break;
    }
  }

  // 2D Navigation: 3 Columns
  int cols = 3;
  int rows = 2;
  int total = 6;

  if (action == Action::Up) {
    // Move up a row (index - cols)
    if (m_menuSelection >= cols)
      m_menuSelection -= cols;
    // Else loop to bottom? Or stay? Let's loop to bottom same col
    else
      m_menuSelection += cols;

    m_soundManager.playOneShot("move", 32);
  } else if (action == Action::Down) {
    // Move down a row (index + cols)
    if (m_menuSelection + cols < total)
      m_menuSelection += cols;
    // Else loop to top
    else
      m_menuSelection -= cols;

    m_soundManager.playOneShot("move", 32);
  } else if (action == Action::Left) {
    // Prev index, wrap around rows?
    // Logic: if at col 0, go to col 2 (same row)
    if (m_menuSelection % cols == 0)
      m_menuSelection += (cols - 1);
    else
      m_menuSelection--;
    m_soundManager.playOneShot("move", 32);
  } else if (action == Action::Right) {
    // Next index, wrap col
    if (m_menuSelection % cols == (cols - 1))
      m_menuSelection -= (cols - 1);
    else
      m_menuSelection++;
    m_soundManager.playOneShot("move", 32);
  }
}

void Game::handleInputPlaceholder(Action action, int mx, int my, bool clicked) {
  // Back Button Logic (Matches renderPlaceholder Back Button)
  int cardH = 300;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  bool hover = (mx >= btnX && mx <= btnX + btnSize && my >= btnY &&
                my <= btnY + btnSize);

  if (hover) {
    m_menuSelection = 0;
    if (clicked) {
      action = Action::Back;
    }
  }

  if (action == Action::Confirm || action == Action::Back) {
    m_state = GameState::MainMenu;
    m_menuSelection = 0;
  }
}

void Game::handleInputOptions(Action action, int mx, int my, bool clicked) {
  // Mouse Detection
  int optionX = 125;
  int optionW = 350;

  int hoverIndex = -1;

  // Sound Area (360-410) - Index 1
  if (mx >= optionX && mx <= optionX + optionW && my >= 360 && my <= 410)
    hoverIndex = 1;

  // Skin Area (430-480) - Index 0
  if (mx >= optionX && mx <= optionX + optionW && my >= 430 && my <= 480)
    hoverIndex = 0;

  // Reset Achv Area (500-550) - Index 2
  // Gap = 70. StartY = 360. Item 2 Y = 360 + 140 = 500.
  int resetW = 220;
  int resetX = (WINDOW_WIDTH - resetW) / 2;
  if (mx >= resetX && mx <= resetX + resetW && my >= 500 && my <= 550)
    hoverIndex = 2;

  // Back Area (Glass Button) - Index 3
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  if (mx >= btnX && mx <= btnX + btnSize && my >= btnY && my <= btnY + btnSize)
    hoverIndex = 3;

  if (hoverIndex != -1) {
    m_menuSelection = hoverIndex;
    if (clicked)
      action = Action::Select;
  }

  if (action == Action::Select) {
    switch (m_menuSelection) {
    case 0: // Skin
      m_darkSkin = !m_darkSkin;
      break;
    case 1: // Sound
      m_soundOn = !m_soundOn;
      m_soundManager.toggleMute();
      break;
    case 2:                                      // Reset Achievements
      m_soundManager.playOneShot("invalid", 64); // Destructive sound
      m_unlockedAchievements = std::vector<bool>(3, false); // Clear Memory
      PersistenceManager::deleteAchievements();             // Clear Disk
      resetGame();                                          // Reset Score/Grid
      break;
    case 3: // Back
      m_state = m_previousState;
      break;
    }
  } else if (action == Action::Back) {
    m_state = m_previousState;
  }

  // Navigation Logic (0, 1, 2, 3)
  // Visual Order: Sound(1) -> Skin(0) -> Reset(2) -> Back(3)
  // Order indices: 1, 0, 2, 3.

  if (action == Action::Up) {
    if (m_menuSelection == 1) // Sound -> Back
      m_menuSelection = 3;
    else if (m_menuSelection == 0) // Skin -> Sound
      m_menuSelection = 1;
    else if (m_menuSelection == 2) // Reset -> Skin
      m_menuSelection = 0;
    else if (m_menuSelection == 3) // Back -> Reset
      m_menuSelection = 2;

    m_soundManager.playOneShot("move", 32);
  } else if (action == Action::Down) {
    if (m_menuSelection == 1) // Sound -> Skin
      m_menuSelection = 0;
    else if (m_menuSelection == 0) // Skin -> Reset
      m_menuSelection = 2;
    else if (m_menuSelection == 2) // Reset -> Back
      m_menuSelection = 3;
    else if (m_menuSelection == 3) // Back -> Sound
      m_menuSelection = 1;

    m_soundManager.playOneShot("move", 32);
  }
}

void Game::handleInputGameOver(Action action, int mx, int my, bool clicked) {
  // Navigation
  if (action == Action::Left || action == Action::Right) {
    // Toggle 0 <-> 1
    m_menuSelection = 1 - m_menuSelection;
    m_soundManager.playOneShot("move", 32);
  }

  // Mouse Detection (Matches renderGameOver Bottom Alignment)
  int cardH = 750;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;

  // BtnY aligned with renderGameOver (Bottom anchored: cardY + cardH - 120)
  int btnY = cardY + cardH - 120;

  int btnSize = 105;
  int gap = 20;
  int totalBtnW = (btnSize * 2) + gap;
  int startX = (WINDOW_WIDTH - totalBtnW) / 2;

  // Try Again (0)
  if (mx >= startX && mx <= startX + btnSize && my >= btnY &&
      my <= btnY + btnSize) {
    m_menuSelection = 0;
    if (clicked)
      action = Action::Confirm;
  }

  // Menu (1)
  int menuX = startX + btnSize + gap;
  if (mx >= menuX && mx <= menuX + btnSize && my >= btnY &&
      my <= btnY + btnSize) {
    m_menuSelection = 1;
    if (clicked)
      action = Action::Confirm;
  }

  if (action == Action::Confirm) {
    if (m_menuSelection == 0) {
      resetGame();
      m_state = GameState::Playing;
    } else {
      m_state = GameState::MainMenu;
      m_menuSelection = 0;
    }
    m_soundManager.playOneShot("spawn", 32);
  }
}
void Game::handleInputPlaying(Action action, int mx, int my, bool clicked) {
  // Check "Back" Button (Bottom Center 105px)
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  if (mx >= btnX && mx <= btnX + btnSize && my >= btnY &&
      my <= btnY + btnSize) {
    if (clicked) {
      m_state = GameState::SavePrompt;
      m_soundManager.playOneShot("move", 32);
      return;
    }
  }

  // Key Back (Esc)
  if (action == Action::Back) {
    m_state = GameState::SavePrompt;
    return;
  }

  if (action == Action::None || action == Action::Quit ||
      action == Action::Restart || action == Action::Confirm)
    return;

  Core::Direction dir;
  if (action == Action::Up)
    dir = Core::Direction::Up;
  else if (action == Action::Down)
    dir = Core::Direction::Down;
  else if (action == Action::Left)
    dir = Core::Direction::Left;
  else if (action == Action::Right)
    dir = Core::Direction::Right;
  else
    return;

  // Execute Logic with MoveEvents
  // Execute Logic with MoveEvents
  auto result = m_logic.move(m_grid, dir);

  if (result.moved) {
    m_score += result.score;
    if (m_score > m_bestScore)
      m_bestScore = m_score;

    // Process Events for Animation
    bool hasAnimations = false;
    for (const auto &evt : result.events) {
      SDL_Rect fromRect = getTileRect(evt.fromX, evt.fromY); // Pixel Coords
      SDL_Rect toRect = getTileRect(evt.toX, evt.toY);

      Animation anim;
      anim.duration = 0.15f; // Slide duration in seconds

      if (evt.type == Core::GameLogic::MoveEvent::Type::Slide ||
          evt.type == Core::GameLogic::MoveEvent::Type::Merge) {

        // Sound: Slide (One Shot per frame)
        m_soundManager.playOneShot("move", 64);

        anim.type = Animation::Type::Slide;
        anim.value = evt.value;
        anim.startX = (float)fromRect.x;
        anim.startY = (float)fromRect.y;
        anim.endX = (float)toRect.x;
        anim.endY = (float)toRect.y;
        anim.startScale = 1.0f;
        anim.endScale = 1.0f;

        m_animationManager.addAnimation(anim);

        // Hide destination until animation arrives
        m_hiddenTiles.insert({evt.toX, evt.toY});

        if (evt.type == Core::GameLogic::MoveEvent::Type::Merge) {
          // Sound: Merge (Allow overlap)
          m_soundManager.play("merge");

          // Visual: Score Popup
          Animation scoreAnim;
          scoreAnim.type = Animation::Type::Score;
          // Center score on the MERGE destination (toRect/endX,endY)
          scoreAnim.startX = (float)toRect.x + (toRect.w / 2.0f);
          scoreAnim.startY = (float)toRect.y;
          scoreAnim.duration = 0.8f;
          scoreAnim.text = "+" + std::to_string(evt.value);

          // Dynamic Color based on tile value
          Color c = getTileColor(evt.value);
          // Use the tile color, but maybe safeguard alpha?
          scoreAnim.color = {c.r, c.g, c.b, 255};

          m_animationManager.addAnimation(scoreAnim);
          m_soundManager.play("score", 64);
        }

        hasAnimations = true;
      }
    }

    // SPAWN NEW TILE
    auto [sx, sy] = m_grid.spawnRandomTile();
    if (sx != -1) {
      // Sound: Spawn
      m_soundManager.play("spawn");

      SDL_Rect sRect = getTileRect(sx, sy);
      Animation spawnAnim;
      spawnAnim.type = Animation::Type::Spawn;
      spawnAnim.value = m_grid.getTile(sx, sy).getValue();
      spawnAnim.startX = (float)sRect.x;
      spawnAnim.startY = (float)sRect.y;
      spawnAnim.endX = (float)sRect.x;
      spawnAnim.endY = (float)sRect.y; // Static pos
      spawnAnim.startScale = 0.0f;
      spawnAnim.endScale = 1.0f;
      spawnAnim.duration = 0.12f; // 120ms

      m_animationManager.addAnimation(spawnAnim);
      m_hiddenTiles.insert({sx, sy});
      hasAnimations = true;
    }

    if (hasAnimations) {
      m_state = GameState::Animating;
    } else {
      if (m_logic.isGameOver(m_grid)) {
        m_state = GameState::GameOver;
        if (PersistenceManager::checkAndSaveHighScore(m_score)) {
          if (m_score > m_bestScore)
            m_bestScore = m_score;
          m_soundManager.playOneShot("score", 128);
        }
        m_soundManager.play("gameover");
        m_menuSelection = 0;
      }
    }
  } else {
    // Invalid Move -> Shake
    m_soundManager.playOneShot("invalid");

    Animation shakeAnim;
    shakeAnim.type = Animation::Type::Shake;
    shakeAnim.duration = 0.3f;
    shakeAnim.shakeOffsetX = 10.0f; // 10px shake magnitude

    m_animationManager.addAnimation(shakeAnim);
    m_state = GameState::Animating; // Block input while shaking
  }
}

void Game::update(float dt) { // Added dt
  m_soundManager.update();    // Reset One-Shot flags

  // Animation State Handling
  m_animationManager.update(
      dt /
      1000.0f); // Always update animations (even non-blocking ones like Score)

  // Check Achievements
  checkAchievements();

  // Popup Timer
  if (m_showAchievementPopup) {
    m_popupTimer -= dt / 1000.0f;
    if (m_popupTimer <= 0.0f) {
      m_showAchievementPopup = false;
    }
  }

  if (m_state == GameState::Animating) {
    if (!m_animationManager.hasBlockingAnimations()) {
      m_state = GameState::Playing;
      m_hiddenTiles
          .clear(); // Show static tiles once blocking animations are done

      // Post-Move Check: Game Over?
      if (m_logic.isGameOver(m_grid)) {
        m_state = GameState::GameOver;
        if (PersistenceManager::checkAndSaveHighScore(m_score)) {
          if (m_score > m_bestScore)
            m_bestScore = m_score;
          m_soundManager.playOneShot("score", 128);
        }
        m_menuSelection = 0; // Reset selection for Game Over menu
      }

      // Spawn new tile if we haven't already in logic?
      // Logic::move DOES NOT SPAWN. We need to spawn manually after move.
      // BUT move() in logic returned result.
      // We should spawn and animate the spawn immediately?
      // YES.
      // Strategy:
      // 1. handleInput calls logic.move.
      // 2. logic.move returns events (Slides/Merges).
      // 3. We queue Slide Animations.
      // 4. We ALSO call grid.spawnRandomTile().
      // 5. We queue Spawn Animation for that new tile.
    }
  }

  switch (m_state) {
  case GameState::Animating:
    break; // Logic handled in if block above
  default:
    break;
  }
}

void Game::render() {
  // 1. Background (Theme Aware)
  Color bg = getBackgroundColor();
  m_renderer.setDrawColor(bg.r, bg.g, bg.b, 255);
  m_renderer.clear();

  switch (m_state) {
  case GameState::MainMenu:
    renderMenu();
    break;
  case GameState::Playing:
  case GameState::Animating: // Render playing state even when animating
    renderPlaying();
    break;
  case GameState::GameOver:
    renderGameOver();
    break;
  case GameState::Options:
    renderOptions();
    break;
  case GameState::LoadGame:
    renderPlaceholder("LOAD GAME");
    break;
  case GameState::BestScores:
    renderBestScores();
    break;
  case GameState::Achievements:
    renderAchievements();
    break;
  case GameState::SavePrompt:
    renderSavePrompt();
    break;
  }

  // Overlay Popup
  if (m_showAchievementPopup) {
    renderAchievementPopup();
  }

  m_renderer.present();
}

void Game::renderMenu() {
  // Phase R: Removed renderGridBackground() to fix "grey placeholders" clutter.
  // The menu is now cleaner on top of the plain window background.

  // Logo
  if (m_logoTexture) {
    // Logo Aspect Ratio Correction
    // Usually 2:1 or Square? User's new logo seems 1:1 or 4:3.
    // Let's assume proportional scaling fitting into a box.
    int logoBoxW = 520; // Increased by 30% (was 400)
    int logoBoxH = 260; // Increased by 30% (was 200)
    int logoW = m_logoTexture->getWidth();
    int logoH = m_logoTexture->getHeight();

    // Scale to fit Box
    float scale = std::min((float)logoBoxW / logoW, (float)logoBoxH / logoH);
    int finalW = (int)(logoW * scale);
    int finalH = (int)(logoH * scale);

    SDL_Rect logoRect = {(WINDOW_WIDTH - finalW) / 2, 80, finalW,
                         finalH}; // Y=80

    m_logoTexture->setColor(255, 255, 255);
    m_renderer.drawTexture(*m_logoTexture, logoRect);
  } else {
    m_renderer.drawTextCentered("TILE TWISTER", m_fontTitle, WINDOW_WIDTH / 2,
                                120, 119, 110, 101, 255);
  }

  const char *options[] = {"Start Game",  "Load Game",    "Options",
                           "Best Scores", "Achievements", "Quit"};

  // Color Mapping (Tile Values)
  // Maps buttons to game colors for thematic consistency
  int values[] = {
      2048, // Start: Gold/Yellow
      1024, // Load:  Gold/Orange
      512,  // Options: Lime/Orange
      128,  // Leaderboard: Cyan
      64,   // Achievements: Red/Pink
      8     // Quit: Orange/White (Low intensity)
  };

  // ULTRA-COMPACT LAYOUT
  int tileSize = 105; // 95 -> 105 (Increase size)
  int gap = 12;       // 15 -> 12

  // Calculate Grid
  int gridW = (tileSize * 3) + (gap * 2);
  // int gridH = (tileSize * 2) + gap;

  // Center Grid Relative to Window/Card
  int startX = (WINDOW_WIDTH - gridW) / 2;
  // Assuming a card-like structure for vertical positioning,
  // we'll define a virtual cardY for consistent layout.
  int cardH = 400; // Example height, adjust as needed for menu layout
  int cardY = (WINDOW_HEIGHT - cardH) / 2;
  int startY = cardY + 235;

  for (int i = 0; i < 6; ++i) {
    int row = i / 3;
    int col = i % 3;

    int btnX = startX + col * (tileSize + gap);
    int btnY = startY + row * (tileSize + gap);

    // Pass the mapped tile value for coloring
    drawGlassButton(i, options[i], btnX, btnY, tileSize, (m_menuSelection == i),
                    values[i]);
  }
}

// Updated Signature with 'value' parameter (treated as Index or Palette Key)
void Game::drawGlassButton(int index, const std::string &text, int x, int y,
                           int size, bool selected, int value) {
  SDL_Rect rect = {x, y, size, size};

  // Animation: Selection Growth
  if (selected) {
    int grow = 8; // More pop
    rect.x -= grow / 2;
    rect.y -= grow / 2;
    rect.w += grow;
    rect.h += grow;
  }

  // Custom Palette for Distinct Colors (Light & Dark)
  // 0: Start (Gold), 1: Load (Blue), 2: Options (Grey/Silver),
  // 3: Leader (Cyan), 4: Achieve (Purple/Pink), 5: Quit (Red)
  // 6: Back (Orange)
  // 7: Try Again (Green)
  // 8: Menu (Blue Grey)
  Color btnColors[9] = {
      {255, 215, 0, 255},   // 0: Gold
      {30, 144, 255, 255},  // 1: Dodger Blue
      {169, 169, 169, 255}, // 2: Options Grey
      {0, 255, 255, 255},   // 3: Cyan
      {255, 105, 180, 255}, // 4: Pink
      {255, 69, 0, 255},    // 5: Red
      {255, 140, 0, 255},   // 6: Back Orange
      {50, 205, 50, 255},   // 7: Restart Green
      {65, 105, 225, 255}   // 8: Menu Blue (Royal Blue)
  };

  Color c = btnColors[index % 9];

  // Determine Contrast Color for Text/Icon
  // Simple luminance formula
  float luminance = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
  bool isBright = luminance > 128;

  // If selected, we use Full Color. Text must contrast.
  // If unselected, we use dimmed color.

  Color contentColor = {255, 255, 255, 255}; // Default White
  if (isBright) {
    contentColor = {50, 50, 50, 255}; // Dark Grey/Black for bright backgrounds
  }

  // 1. Background (Glass)
  if (m_tileTexture) {
    m_tileTexture->setBlendMode(
        SDL_BLENDMODE_BLEND); // Always Blend for visibility

    m_tileTexture->setColor(c.r, c.g, c.b);

    if (selected) {
      m_tileTexture->setAlpha(255); // Solid Active
    } else {
      // Unselected: Ghostly but visible color
      // In Dark Mode, we must ensure it's not too dark
      m_tileTexture->setAlpha(150);
    }

    m_renderer.drawTexture(*m_tileTexture, rect);
  } else {
    m_renderer.setDrawColor(c.r, c.g, c.b, selected ? 255 : 150);
    m_renderer.drawFillRect(rect.x, rect.y, rect.w, rect.h);
  }

  // 2. Icons
  // Use contentColor
  SDL_SetRenderDrawColor(m_renderer.getInternal(), contentColor.r,
                         contentColor.g, contentColor.b, 255);

  int iconSize = size / 2;
  int iconX = rect.x + (rect.w - iconSize) / 2;
  int iconY = rect.y + 15;

  drawProceduralIcon(m_renderer.getInternal(), index, iconX, iconY, iconSize,
                     contentColor.r, contentColor.g, contentColor.b, 255);

  // 3. Label text (Tiny Font)
  int textY = rect.y + size - 20;

  m_renderer.drawTextCentered(text, m_fontTiny, rect.x + rect.w / 2, textY,
                              contentColor.r, contentColor.g, contentColor.b,
                              255);
}

void Game::renderOptions() {
  // Background
  renderGridBackground();

  // Card Panel
  int cardW = 500;
  int cardH = 400;
  int cardX = (WINDOW_WIDTH - cardW) / 2;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;
  drawCard(cardX, cardY, cardW, cardH);

  // Header
  uint8_t r = m_darkSkin ? 119 : 60;
  uint8_t g = m_darkSkin ? 110 : 60;
  uint8_t b = m_darkSkin ? 101 : 60;
  m_renderer.drawTextCentered("OPTIONS", m_fontTitle, WINDOW_WIDTH / 2,
                              cardY + 70, r, g, b, 255);

  int startY = cardY + 160;
  int optionW = 350;
  int optionX = (WINDOW_WIDTH - optionW) / 2;
  int gap = 70;

  // 1. Sound Toggle
  drawSwitch("Sound", m_soundOn, optionX, startY, optionW,
             (m_menuSelection == 1));

  // 2. Skin Toggle
  drawSwitch(m_darkSkin ? "Dark Mode" : "Light Mode", m_darkSkin, optionX,
             startY + gap, optionW, (m_menuSelection == 0));

  // 3. Reset Achievements (Button)
  int resetW = 220;
  int resetX = (WINDOW_WIDTH - resetW) / 2;
  int resetY = startY + (gap * 2);
  drawButton("Reset Achv", resetX, resetY, resetW, 50, (m_menuSelection == 2));

  // 4. Back Button (Glass Style)
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  // Use 6 (Back Arrow + Orange)
  drawGlassButton(6, "Back", btnX, btnY, btnSize, (m_menuSelection == 3), 6);
}

void Game::renderPlaceholder(const std::string &title) {
  renderGridBackground();

  int cardW = 500;
  int cardH = 300;
  int cardX = (WINDOW_WIDTH - cardW) / 2;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;

  drawCard(cardX, cardY, cardW, cardH);

  uint8_t r = m_darkSkin ? 119 : 60;
  uint8_t g = m_darkSkin ? 110 : 60;
  uint8_t b = m_darkSkin ? 101 : 60;

  m_renderer.drawTextCentered(title, m_fontTitle, WINDOW_WIDTH / 2, cardY + 80,
                              r, g, b, 255);

  m_renderer.drawTextCentered("Coming Soon...", m_fontMedium, WINDOW_WIDTH / 2,
                              cardY + 160, r, g, b, 150);

  // Back Button (Glass Style)
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  drawGlassButton(6, "Back", btnX, btnY, btnSize, (m_menuSelection == 0), 6);
}

void Game::renderGameOver() {
  // Render Board (Background)
  renderPlaying();

  // Overlay Result Card
  // Massive Card for Massive Logo
  int cardW = 500;
  int cardH = 750;
  int cardX = (WINDOW_WIDTH - cardW) / 2;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;

  // Draw Card (Dimmer)
  drawCard(cardX, cardY, cardW, cardH);

  // Vertical layout cursor
  int curY = cardY + 20;

  // 1. Logo (Massive 3x)
  if (m_logoTexture) {
    int logoH = 250; // Requested "3x bigger"
    int w = m_logoTexture->getWidth();
    int h = m_logoTexture->getHeight();
    float aspect = (float)w / h;
    int logoW = (int)(logoH * aspect);
    // Clamp width if too wide for card
    if (logoW > cardW - 20) {
      logoW = cardW - 20;
      logoH = (int)(logoW / aspect);
    }
    int logoX = (WINDOW_WIDTH - logoW) / 2;

    SDL_Rect logoRect = {logoX, curY, logoW, logoH};
    m_logoTexture->setColor(255, 255, 255);
    m_renderer.drawTexture(*m_logoTexture, logoRect);
    curY += logoH + 30; // Spacing
  } else {
    curY += 50;
  }

  // 2. Title "GAME OVER!"
  // Ensure we are below Logo
  std::string title = "GAME OVER!";
  int charW = 20;
  int totalW = title.length() * charW;
  int txtX = (WINDOW_WIDTH - totalW) / 2;

  int valScale = 2;
  for (char c : title) {
    valScale *= 2;
    if (valScale > 2048)
      valScale = 2;
  }
  curY += 50;

  // 3. Score
  std::string scoreTxt = std::to_string(m_score);
  // Color: Bright Green for visibility
  m_renderer.drawTextCentered(scoreTxt, m_fontTitle, WINDOW_WIDTH / 2, curY, 0,
                              200, 0, 255);
  curY += 70; // Increased spacing to "Final Score"

  Color labelColor =
      m_darkSkin ? Color{200, 200, 200, 255} : Color{119, 110, 101, 255};
  m_renderer.drawTextCentered("Final Score", m_fontMedium, WINDOW_WIDTH / 2,
                              curY, labelColor.r, labelColor.g, labelColor.b,
                              255);

  // 4. Buttons (Glass Style)
  // Two 105px buttons. Gap 20.
  int btnSize = 105;
  int gap = 20;
  int totalBtnW = (btnSize * 2) + gap;
  int btnStartX = (WINDOW_WIDTH - totalBtnW) / 2;
  int btnY = cardY + cardH - 120; // Bottom alignment

  // Button 1: Try Again (Index 7: Green)
  drawGlassButton(7, "Try Again", btnStartX, btnY, btnSize,
                  (m_menuSelection == 0), 7);

  // Button 2: Menu (Index 8: Blue Grey)
  drawGlassButton(8, "Menu", btnStartX + btnSize + gap, btnY, btnSize,
                  (m_menuSelection == 1), 8);
}

void Game::renderScoreBox(const std::string &label, int value, int x, int y) {
  int boxW = 80; // Compact
  int boxH = 55;

  // Background
  SDL_Rect rect = {x, y, boxW, boxH};
  Color boxColor = {187, 173, 160, 255}; // #bbada0
  m_renderer.setDrawColor(boxColor.r, boxColor.g, boxColor.b, 255);

  if (m_tileTexture) {
    m_tileTexture->setColor(boxColor.r, boxColor.g, boxColor.b);
    m_renderer.drawTexture(*m_tileTexture, rect);
  } else {
    m_renderer.drawFillRect(rect.x, rect.y, rect.w, rect.h);
  }

  // Label "SCORE" or "BEST"
  Color labelColor = {238, 228, 218, 255}; // #eee4da
  m_renderer.drawTextCentered(label, m_fontSmall, x + boxW / 2, y + 15,
                              labelColor.r, labelColor.g, labelColor.b, 255);

  // Value
  Color valueColor = {255, 255, 255, 255};
  m_renderer.drawTextCentered(std::to_string(value), m_fontMedium, x + boxW / 2,
                              y + 38, valueColor.r, valueColor.g, valueColor.b,
                              255);
}

void Game::renderHeader() {
  int headerY = 30; // 20->30 for spacing

  // 1. Logo (Replaces "2048" Text)
  if (m_logoTexture) {
    // Scale Logo to fit Header height ~80px
    int logoH = 80;
    int w = m_logoTexture->getWidth();
    int h = m_logoTexture->getHeight();
    float aspect = (float)w / h;
    int logoW = (int)(logoH * aspect);

    SDL_Rect logoRect = {20, headerY - 10, logoW, logoH};

    // Ensure logo pops logic
    m_logoTexture->setColor(255, 255, 255);
    m_renderer.drawTexture(*m_logoTexture, logoRect);
  } else {
    Color titleColor = {119, 110, 101, 255};
    m_renderer.drawText("2048", m_fontTitle, 20, headerY - 10, titleColor.r,
                        titleColor.g, titleColor.b, 255);
  }

  // 2. Score Boxes
  // Align Right
  int boxW = 80;
  int margin = 10;
  int startX = WINDOW_WIDTH - (boxW * 2) - margin - 20;

  renderScoreBox("SCORE", m_score, startX, headerY);
  renderScoreBox("BEST", m_bestScore, startX + boxW + margin, headerY);

  // Subtext: "Join the numbers and get to the 2048 tile!" (Optional, maybe
  // later)
}

void Game::renderPlaying() {
  // 1. Render Header (includes HUD)
  renderHeader();

  // 1.5 Render Toolbar (Restart / Options)
  int toolbarY = 120;
  Color btnColor = {119, 110, 101, 255}; // #776e65
  // Used m_fontMedium (Size 30) for prominence
  m_renderer.drawText("Restart", m_fontMedium, 20, toolbarY + 5, btnColor.r,
                      btnColor.g, btnColor.b, 255);
  m_renderer.drawText("Options", m_fontMedium, 460, toolbarY + 5, btnColor.r,
                      btnColor.g, btnColor.b, 255);

  // Rounded Box for buttons visual cue? (Optional)
  // SDL_Rect rBtn = {20, toolbarY, 100, 40};
  // SDL_Rect oBtn = {480, toolbarY, 100, 40};
  // m_renderer.setDrawColor(187, 173, 160, 255); // Bg color
  // But strictly standard 2048 usually has text buttons or smaller boxes.
  // Reading file first "Clean" look.

  // --- Calculate Shake Offset ---
  int shakeX = 0;
  for (const auto &anim : m_animationManager.getAnimations()) {
    if (anim.type == Animation::Type::Shake) {
      float t = anim.getProgress();
      float decay = 1.0f - t;
      // Simple sine shake: 3 cycles * decay
      shakeX =
          static_cast<int>(std::sin(t * 20.0f) * anim.shakeOffsetX * decay);
    }
  }

  // 2. Render Grid Background
  Color gridColor = getGridColor();

  // Layout V2: Y=180, Size=450
  int gridY = 180;
  int gridSize = 450;
  int marginX = (WINDOW_WIDTH - gridSize) / 2; // (600-450)/2 = 75

  // Background Removed (Transparent Board) but logic remains here

  // 3. Render Tiles
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      // SKIP rendering if this tile is currently being animated (target of
      // animation) Note: We hide the TARGET of the slide.
      if (m_hiddenTiles.count({x, y}))
        continue;

      Core::Tile tile = m_grid.getTile(x, y);

      SDL_Rect rect = getTileRect(x, y); // Use the helper
      rect.x += shakeX;                  // Apply Shake

      Color c =
          tile.isEmpty() ? getEmptyTileColor() : getTileColor(tile.getValue());

      if (m_tileTexture) {
        m_tileTexture->setColor(c.r, c.g, c.b);
        m_renderer.drawTexture(*m_tileTexture, rect);
      } else {
        m_renderer.setDrawColor(c.r, c.g, c.b, c.a);
        m_renderer.drawFillRect(rect.x, rect.y, rect.w, rect.h);
      }

      if (!tile.isEmpty()) {
        Color tc = getTextColor(tile.getValue());
        m_renderer.drawTextCentered(std::to_string(tile.getValue()), m_font,
                                    rect.x + rect.w / 2, rect.y + rect.h / 2,
                                    tc.r, tc.g, tc.b, tc.a);
      }
    }
  }

  // 4. Render Animations (Slide/Spawn/Merge/Score)
  for (const auto &anim : m_animationManager.getAnimations()) {
    if (anim.type == Animation::Type::Shake)
      continue; // Skip shake logic

    // Float/Score Logic
    if (anim.type == Animation::Type::Score) {
      float t = anim.getProgress();
      // Float Up by 50px
      float curY = anim.startY - (50.0f * t);
      float curX = anim.startX;

      // Center on the tile position provided (assume startX/Y are Tile Coords
      // not Pixels?) Wait, handleInput puts Pixel coords or Tile coords?
      // Usually we want Pixel coords. We will handle that in handleInput.
      // Assuming input is Pixel start coords.

      SDL_Rect rect = {
          (int)curX + shakeX, (int)curY, 0,
          0}; // Apply Shake? Maybe not for float text to keep it decoupled?
      // If board shakes, scores usually shake too.

      uint8_t alpha = static_cast<uint8_t>(255 * (1.0f - t));

      // Render Text
      m_renderer.drawTextCentered(anim.text, m_fontMedium, rect.x, rect.y,
                                  anim.color.r, anim.color.g, anim.color.b,
                                  alpha);
      continue;
    }

    // Slide/Spawn Logic
    float t = anim.getProgress();
    float curX = anim.startX + (anim.endX - anim.startX) * t;
    float curY = anim.startY + (anim.endY - anim.startY) * t;

    // Scaling for Spawn
    float curScale = anim.startScale + (anim.endScale - anim.startScale) * t;

    SDL_Rect r;
    SDL_Rect sz = getTileRect(0, 0); // Base size
    int w = static_cast<int>(sz.w * curScale);
    int h = static_cast<int>(sz.h * curScale);

    // Center the scaled rect
    int finalX = (int)curX + (sz.w - w) / 2 + shakeX; // Apply Shake
    int finalY = (int)curY + (sz.h - h) / 2;

    r.x = finalX;
    r.y = finalY;
    r.w = w;
    r.h = h;

    // Render
    Color c = getTileColor(anim.value);

    // Alpha for Merge?
    // Just solid for now.

    if (m_tileTexture) {
      m_tileTexture->setColor(c.r, c.g, c.b);
      m_renderer.drawTexture(*m_tileTexture, r);
    } else {
      m_renderer.setDrawColor(c.r, c.g, c.b, 255);
      m_renderer.drawFillRect(r.x, r.y, r.w, r.h);
    }

    // Text
    Color tc = getTextColor(anim.value);
    m_renderer.drawTextCentered(std::to_string(anim.value), m_font,
                                r.x + r.w / 2, r.y + r.h / 2, tc.r, tc.g, tc.b,
                                255);
  }

  // 5. Back Button (Glass Style)
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;
  drawGlassButton(6, "Back", btnX, btnY, btnSize, false, 6);
}

// Colors
Color Game::getBackgroundColor() const {
  return m_darkSkin ? Color{51, 51, 51, 255} : Color{250, 248, 239, 255};
}
// --- UI HELPERS ---

void Game::drawOverlay() {
  // Full Screen Dimmer
  // Light Mode: Very light beige tint with high alpha? Or dark?
  // User wants "Glass" effect.
  // Dark Mode: Dark overlay.

  if (m_darkSkin) {
    m_renderer.setDrawColor(30, 30, 30, 240); // Almost opaque dark
  } else {
    m_renderer.setDrawColor(250, 248, 239, 240); // Almost opaque light
  }

  // Draw over entire window
  m_renderer.drawFillRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void Game::drawCard(int x, int y, int w, int h) {
  // Legacy or specific use
  drawOverlay();
}

void Game::drawButton(const std::string &text, int x, int y, int w, int h,
                      bool selected) {
  SDL_Rect rect = {x, y, w, h};

  // Hover/Selected Animation Effect
  if (selected) {
    // Enlarge slightly
    int growth = 4;
    rect.x -= growth / 2;
    rect.y -= growth / 2;
    rect.w += growth;
    rect.h += growth;
  }

  // Background Color Logic
  // Selected: Orange (#f67c5f), Normal: Brown (#8f7a66)
  Color btnColor =
      selected ? Color{246, 124, 95, 255} : Color{143, 122, 102, 255};

  if (m_buttonTexture) {
    // Use the dedicated button texture (capsule)
    // Tint it to match the state color
    m_buttonTexture->setColor(btnColor.r, btnColor.g, btnColor.b);
    m_renderer.drawTexture(*m_buttonTexture, rect);
  } else {
    // Fallback: Clean Flat Design (No stretched tile)
    m_renderer.setDrawColor(btnColor.r, btnColor.g, btnColor.b, 255);
    m_renderer.drawFillRect(rect.x, rect.y, rect.w, rect.h);
  }

  // Text
  m_renderer.drawTextCentered(text, m_fontMedium, rect.x + rect.w / 2,
                              rect.y + rect.h / 2 - 2, 255, 255, 255, 255);
}

void Game::drawSwitch(const std::string &label, bool value, int x, int y, int w,
                      bool selected) {
  // Label
  uint8_t r = m_darkSkin ? 249 : 119;
  uint8_t g = m_darkSkin ? 246 : 110;
  uint8_t b = m_darkSkin ? 242 : 101;

  m_renderer.drawText(label, m_fontMedium, x, y, r, g, b, selected ? 255 : 150);

  // Switch Graphic (Right Aligned in width w)
  int switchW = 60;
  int switchH = 30;
  int switchX = x + w - switchW;
  int switchY = y;

  // Background (Track)
  Color trackColor = value ? Color{246, 124, 95, 255}
                           : Color{200, 200, 200, 255}; // Orange or Grey

  // Draw rounded rect using tile texture if possible
  SDL_Rect trackRect = {switchX, switchY, switchW, switchH};
  if (m_tileTexture) {
    m_tileTexture->setColor(trackColor.r, trackColor.g, trackColor.b);
    m_renderer.drawTexture(*m_tileTexture, trackRect);
  } else {
    m_renderer.setDrawColor(trackColor.r, trackColor.g, trackColor.b, 255);
    m_renderer.drawFillRect(switchX, switchY, switchW, switchH);
  }

  // Knob
  int knobSize = 26;
  int knobX = value ? (switchX + switchW - knobSize - 2) : (switchX + 2);
  int knobY = switchY + 2;

  m_renderer.setDrawColor(255, 255, 255, 255);
  m_renderer.drawFillRect(knobX, knobY, knobSize, knobSize);
}

// --- RENDER HELPERS ---

void Game::renderGridBackground() {
  Color gridColor = getGridColor();

  // Layout V2: Y=180, Size=450
  int gridY = 180;
  int gridSize = 450;
  int marginX = (WINDOW_WIDTH - gridSize) / 2;

  // Check if tile texture is loaded to use for background tiles
  // Actually, we removed the full background and used transparent board.
  // But for the menus, we want to see the TILES in the background if game is
  // playing? Or just empty grid? Let's render the Playing state logic for tiles
  // but dimmed? Re-using renderPlaying() is tricky if we want blur. Let's just
  // render the static empty grid slots for visual context.

  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      SDL_Rect rect = getTileRect(x, y);
      Color c = getEmptyTileColor();

      if (m_tileTexture) {
        m_tileTexture->setColor(c.r, c.g, c.b);
        m_renderer.drawTexture(*m_tileTexture, rect);
      } else {
        m_renderer.setDrawColor(c.r, c.g, c.b, 255);
        m_renderer.drawFillRect(rect.x, rect.y, rect.w, rect.h);
      }
    }
  }
}

// Helpers
Color Game::getGridColor() const {
  return m_darkSkin ? Color{77, 77, 77, 255} : Color{187, 173, 160, 255};
}
Color Game::getEmptyTileColor() const {
  return m_darkSkin ? Color{89, 89, 89, 255} : Color{205, 193, 180, 255};
}

Color Game::getTileColor(int val) const {
  if (m_darkSkin) {
    // Neon Palette (Brightened)
    switch (val) {
    case 2:
      return {0, 255, 255, 255}; // Cyan (Max Brightness)
    case 4:
      return {0, 191, 255, 255}; // Deep Sky Blue
    case 8:
      return {255, 255, 0, 255}; // Yellow
    case 16:
      return {255, 165, 0, 255}; // Orange
    case 32:
      return {255, 80, 0, 255}; // Red-Orange
    case 64:
      return {255, 20, 147, 255}; // Deep Pink
    case 128:
      return {57, 255, 20, 255}; // Neon Green
    case 256:
      return {0, 255, 127, 255}; // Spring Green
    case 512:
      return {255, 0, 255, 255}; // Magenta
    default:
      return {255, 255, 255, 255}; // White
    }
  } else {
    // Classic Light Palette
    switch (val) {
    case 2:
      return {238, 228, 218, 255};
    case 4:
      return {237, 224, 200, 255};
    case 8:
      return {242, 177, 121, 255};
    case 16:
      return {245, 149, 99, 255};
    case 32:
      return {246, 124, 95, 255};
    case 64:
      return {246, 94, 59, 255};
    case 128:
      return {237, 207, 114, 255};
    case 256:
      return {237, 204, 97, 255};
    case 512:
      return {237, 200, 80, 255};
    case 1024:
      return {237, 197, 63, 255};
    case 2048:
      return {237, 194, 46, 255};
    default:
      return {60, 58, 50, 255};
    }
  }
}

Color Game::getTextColor(int val) const {
  return (val <= 4) ? Color{119, 110, 101, 255} : Color{249, 246, 242, 255};
}

void Game::resetGame() {
  m_grid = Core::Grid();
  m_grid.spawnRandomTile();
  m_score = 0;
}

SDL_Rect Game::getTileRect(int x, int y) const {
  int gridY = 180;
  int gridSize = 450;
  int marginX = (WINDOW_WIDTH - gridSize) / 2; // 75
  int padding = 15;
  int titleSize = (gridSize - 5 * padding) / 4;
  int xPos = marginX + padding + x * (titleSize + padding);
  int yPos = gridY + padding + y * (titleSize + padding);
  return {xPos, yPos, titleSize, titleSize};
}

void Game::handleInputSavePrompt(Action action, int mx, int my, bool clicked) {
  // Simple Yes/No Overlay logic
  // Center Card: 400x300.
  // Buttons: Yes (Green), No (Red).
  int cardW = 400;
  int cardH = 300;
  int cardX = (WINDOW_WIDTH - cardW) / 2;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;

  int btnWidth = 120;
  int btnHeight = 60;
  int gap = 40;
  int startY = cardY + 180;
  int startX = (WINDOW_WIDTH - (btnWidth * 2 + gap)) / 2;

  // Yes (Left)
  if (mx >= startX && mx <= startX + btnWidth && my >= startY &&
      my <= startY + btnHeight) {
    if (clicked) {
      PersistenceManager::saveGame(m_grid, m_score);
      m_state = GameState::MainMenu;
      m_menuSelection = 0;
      m_soundManager.playOneShot("score", 64);
    }
  }

  // No (Right)
  int noX = startX + btnWidth + gap;
  if (mx >= noX && mx <= noX + btnWidth && my >= startY &&
      my <= startY + btnHeight) {
    if (clicked) {
      m_state = GameState::MainMenu; // Quit without saving (or Back to Game?)
      m_menuSelection = 0;
      m_soundManager.playOneShot("move", 64);
    }
  }

  // Back / Escape -> Return to Game
  if (action == Action::Back) {
    m_state = GameState::Playing;
  }
}

void Game::handleInputBestScores(Action action, int mx, int my, bool clicked) {
  // Back Button Logic
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  if (mx >= btnX && mx <= btnX + btnSize && my >= btnY &&
      my <= btnY + btnSize) {
    if (clicked)
      action = Action::Back;
  }

  if (action == Action::Back || action == Action::Confirm) {
    m_state = GameState::MainMenu;
    m_menuSelection = 0;
  }
}

void Game::renderSavePrompt() {
  // Dim background
  renderPlaying(); // Render game behind
  drawOverlay();

  int cardW = 400;
  int cardH = 300;
  int cardX = (WINDOW_WIDTH - cardW) / 2;
  int cardY = (WINDOW_HEIGHT - cardH) / 2;

  drawCard(cardX, cardY, cardW, cardH);

  Color textRGB =
      m_darkSkin ? Color{255, 255, 255, 255} : Color{119, 110, 101, 255};
  Color subRGB =
      m_darkSkin ? Color{200, 200, 200, 255} : Color{150, 140, 130, 255};

  m_renderer.drawTextCentered("Save Progress?", m_fontMedium, WINDOW_WIDTH / 2,
                              cardY + 60, textRGB.r, textRGB.g, textRGB.b, 255);
  m_renderer.drawTextCentered("Unsaved data will be lost.", m_fontSmall,
                              WINDOW_WIDTH / 2, cardY + 110, subRGB.r, subRGB.g,
                              subRGB.b, 255);

  int size = 105;
  int gap = 30;
  int startX = (WINDOW_WIDTH - (size * 2 + gap)) / 2;
  int startY = cardY + 160;

  // Yes: Green (Index 7 - Restart/Tick from Game Over)
  drawGlassButton(7, "Save", startX, startY, size, false, 7);

  // No: Red (Index 5 - Quit)
  drawGlassButton(5, "Quit", startX + size + gap, startY, size, false, 5);
}

void Game::renderBestScores() {
  renderHeader();

  int cardW = 540; // Wider
  int cardH = 500;
  int cardY = 220;
  int cardX = (WINDOW_WIDTH - cardW) / 2;

  drawCard(cardX, cardY, cardW, cardH);

  Color textRGB =
      m_darkSkin ? Color{255, 255, 255, 255} : Color{119, 110, 101, 255};
  Color headColor =
      m_darkSkin ? Color{200, 200, 200, 255} : Color{143, 122, 102, 255};

  m_renderer.drawTextCentered("BEST SCORES", m_fontTitle, WINDOW_WIDTH / 2, 100,
                              119, 110, 101, 255);

  // Headers
  int listY = cardY + 30;
  m_renderer.drawText("Date", m_fontSmall, cardX + 30, listY, headColor.r,
                      headColor.g, headColor.b, 255);
  m_renderer.drawText("Score", m_fontSmall, cardX + 400, listY, headColor.r,
                      headColor.g, headColor.b, 255);

  listY += 50; // More gap

  auto scores = PersistenceManager::loadLeaderboard();
  if (scores.empty()) {
    m_renderer.drawTextCentered("No records yet.", m_fontMedium,
                                WINDOW_WIDTH / 2, cardY + 200, textRGB.r,
                                textRGB.g, textRGB.b, 150);
  } else {
    // Animation Pulse
    float time = SDL_GetTicks() / 1000.0f;
    float pulse = (sin(time * 3.0f) + 1.0f) * 0.5f; // 0..1

    int rank = 0;
    for (const auto &entry : scores) {
      rank++;
      if (rank > 5)
        break;

      // Date
      m_renderer.drawText(entry.date, m_fontSmall, cardX + 30, listY + 5,
                          textRGB.r, textRGB.g, textRGB.b, 255);

      // Score (Rightish)
      m_renderer.drawText(std::to_string(entry.score), m_fontMedium,
                          cardX + 400, listY, 255, 215, 0, 255);

      // Render Stars (Center)
      if (m_starTexture) {
        int starCount = 6 - rank;
        int baseSize = 24;
        int gap = 2;
        int startStarX = cardX + 220; // Explicit region

        // Gold Pulse Color
        Uint8 glowA = (Uint8)(100 + pulse * 155);

        for (int s = 0; s < starCount; ++s) {
          int sx = startStarX + s * (baseSize + gap);
          int sy = listY;

          // Glow Pass (Back, Larger, Alpha)
          if (rank <= 3) { // Only top 3 glow
            SDL_Rect gRect = {sx - 4, sy - 4, baseSize + 8, baseSize + 8};
            m_starTexture->setBlendMode(SDL_BLENDMODE_ADD);
            m_starTexture->setColor(255, 200, 50);
            m_starTexture->setAlpha(glowA / 2);
            m_renderer.drawTexture(*m_starTexture, gRect);
            m_starTexture->setBlendMode(SDL_BLENDMODE_BLEND); // Reset
          }

          // Main Star
          SDL_Rect sRect = {sx, sy, baseSize, baseSize};
          m_starTexture->setColor(255, 215, 0);
          m_starTexture->setAlpha(255);
          m_renderer.drawTexture(*m_starTexture, sRect);
        }
      }

      listY += 60; // Taller rows
    }
  }

  // Back Button
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;
  drawGlassButton(6, "Back", btnX, btnY, btnSize, false, 6);
}

void Game::checkAchievements() {
  int milestones[] = {500, 1000, 2000};
  bool changed = false;
  for (int i = 0; i < 3; ++i) {
    if (!m_unlockedAchievements[i] && m_score >= milestones[i]) {
      m_unlockedAchievements[i] = true;
      m_showAchievementPopup = true;
      m_popupAchievementIndex = i;
      m_popupTimer = 4.0f;                          // 4 Seconds
      m_soundManager.playOneShot("fireworks", 128); // Real fireworks
      changed = true;
    }
  }
  if (changed) {
    PersistenceManager::saveAchievements(m_unlockedAchievements);
  }
}

void Game::renderAchievementPopup() {
  // Top center notification
  int w = 400;
  int h = 100;
  int x = (WINDOW_WIDTH - w) / 2;
  int y = 50; // Slide down?

  // Animate Y based on timer? 4s total.
  // 0-0.5s slide in. 3.5-4.0s slide out.
  float t = 4.0f - m_popupTimer; // Elapsed
  if (t < 0.5f) {
    float p = t / 0.5f;
    y = -100 + (150 * p); // Slide down to 50
  } else if (t > 3.5f) {
    float p = (t - 3.5f) / 0.5f;
    y = 50 - (150 * p); // Slide up
  }

  // Draw Box
  SDL_Rect rect = {x, y, w, h};
  m_renderer.setDrawColor(30, 30, 30, 120); // More transparent (120/255)
  m_renderer.drawFillRect(rect.x, rect.y, rect.w, rect.h);

  // Icon
  if (m_popupAchievementIndex >= 0 &&
      m_popupAchievementIndex < m_achievementTextures.size()) {
    auto &tex = m_achievementTextures[m_popupAchievementIndex];
    if (tex) {
      SDL_Rect ir = {x + 20, y + 10, 80, 80};
      tex->setColor(255, 255, 255);
      m_renderer.drawTexture(*tex, ir);
    }
  }

  // Text
  m_renderer.drawText("Achievement Unlocked!", m_fontSmall, x + 120, y + 25,
                      255, 215, 0, 255);
  std::string names[] = {"Bronze Medal", "Silver Cup", "Super Cup"};
  if (m_popupAchievementIndex >= 0 && m_popupAchievementIndex < 3)
    m_renderer.drawText(names[m_popupAchievementIndex], m_fontMedium, x + 120,
                        y + 50, 255, 255, 255, 255);
}

void Game::renderAchievements() {
  renderHeader();
  // Grid 1x3 vertical
  int cardW = 500;
  int cardH = 600;
  int cardX = (WINDOW_WIDTH - cardW) / 2;
  int cardY = 150;

  drawCard(cardX, cardY, cardW, cardH);

  Color titleColor =
      m_darkSkin ? Color{255, 255, 255, 255} : Color{119, 110, 101, 255};
  m_renderer.drawTextCentered("ACHIEVEMENTS", m_fontTitle, WINDOW_WIDTH / 2, 60,
                              titleColor.r, titleColor.g, titleColor.b, 255);

  int itemH = 150;
  int curY = cardY + 50;
  std::string titles[] = {"500 Points", "1000 Points", "2000 Points"};
  std::string desc[] = {"Bronze Medal", "Silver Cup", "Super Cup"};

  for (int i = 0; i < 3; ++i) {
    bool unlocked = m_unlockedAchievements[i];

    // Icon
    int iconSize = 100;
    int iconX = cardX + 50;
    int iconY = curY + (itemH - iconSize) / 2;

    if (i < m_achievementTextures.size() && m_achievementTextures[i]) {
      auto &tex = m_achievementTextures[i];
      SDL_Rect r = {iconX, iconY, iconSize, iconSize};
      if (unlocked) {
        tex->setColor(255, 255, 255);
      } else {
        tex->setColor(80, 80, 80); // Silhouette
      }
      m_renderer.drawTexture(*tex, r);
    }

    // Text
    int textX = iconX + iconSize + 30;
    Color tc =
        m_darkSkin ? Color{255, 255, 255, 255} : Color{119, 110, 101, 255};
    m_renderer.drawText(desc[i], m_fontMedium, textX, curY + 40, tc.r, tc.g,
                        tc.b, unlocked ? 255 : 100);
    m_renderer.drawText(titles[i], m_fontSmall, textX, curY + 80, tc.r, tc.g,
                        tc.b, unlocked ? 255 : 100);

    curY += itemH;
  }

  // Back Button
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;
  drawGlassButton(6, "Back", btnX, btnY, btnSize, false, 6);
}

void Game::handleInputAchievements(Action action, int mx, int my,
                                   bool clicked) {
  int btnSize = 105;
  int btnX = (WINDOW_WIDTH - btnSize) / 2;
  int btnY = WINDOW_HEIGHT - 160;

  if (mx >= btnX && mx <= btnX + btnSize && my >= btnY &&
      my <= btnY + btnSize) {
    if (clicked)
      action = Action::Back;
  }

  if (action == Action::Back) {
    m_state = GameState::MainMenu;
    m_menuSelection = 0;
  }
}

} // namespace Game
