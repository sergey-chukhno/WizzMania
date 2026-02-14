#pragma once
#include "../core/GameLogic.hpp"
#include "../core/Grid.hpp"
#include "../engine/Context.hpp"
#include "../engine/Font.hpp"
#include "../engine/Renderer.hpp"
#include "../engine/SoundManager.hpp"
#include "../engine/Texture.hpp"
#include "../engine/Window.hpp"
#include "AnimationManager.hpp" // Added
#include "InputManager.hpp"     // Added
#include <set>                  // Added

namespace Game {

enum class GameState {
  MainMenu,
  Playing,
  Animating,
  GameOver,
  Options,
  BestScores,
  Achievements,
  LoadGame,
  SavePrompt
};

class Game {
public:
  Game();
  ~Game() = default;

  void run();

private:
  void handleInput();
  void update(float dt);
  void render();

  // State Handlers
  void handleInputMenu(Action action, int mx, int my, bool clicked);
  void handleInputPlaying(Action action, int mx, int my, bool clicked);
  void handleInputGameOver(Action action, int mx, int my, bool clicked);
  void handleInputOptions(Action action, int mx, int my, bool clicked);
  void handleInputSavePrompt(Action action, int mx, int my, bool clicked);
  void handleInputBestScores(Action action, int mx, int my, bool clicked);
  void handleInputAchievements(Action action, int mx, int my, bool clicked);
  void handleInputPlaceholder(Action action, int mx, int my,
                              bool clicked); // For Load/Achievements

  void renderMenu();
  void renderPlaying();
  void renderGameOver();
  void renderOptions();
  void renderSavePrompt();
  void renderBestScores();
  void renderAchievements();
  void renderAchievementPopup();
  void renderPlaceholder(const std::string &title);

  void resetGame();

  // Scoring
  int m_score;
  int m_bestScore;

  // Rendering Helpers
  [[nodiscard]] Color getTileColor(int value) const;
  [[nodiscard]] SDL_Rect getTileRect(int x, int y) const;

  // Visual Overhaul
  std::unique_ptr<Engine::Texture> m_tileTexture;
  std::unique_ptr<Engine::Texture> m_logoTexture;
  std::unique_ptr<Engine::Texture> m_buttonTexture;
  std::unique_ptr<Engine::Texture> m_starTexture;
  std::vector<std::unique_ptr<Engine::Texture>> m_achievementTextures;
  std::unique_ptr<Engine::Texture> m_glassTileTexture; // For Menu Grid
  std::unique_ptr<Engine::Texture> m_iconsTexture;     // For Menu Icons

  void renderHeader();
  void renderScoreBox(const std::string &label, int value, int x, int y);
  void renderGridBackground();

  // UI Helpers
  void drawOverlay(); // Full screen dimmer
  void drawButton(const std::string &text, int x, int y, int w, int h,
                  bool selected);
  void drawGlassButton(int index, const std::string &text, int x, int y,
                       int size, bool selected, int value); // New Grid Button
  void drawCard(int x, int y, int w, int h); // Keep for placeholder?
  void drawSwitch(const std::string &label, bool value, int x, int y, int w,
                  bool selected);

  [[nodiscard]] Color getBackgroundColor() const;
  [[nodiscard]] Color getGridColor() const;
  [[nodiscard]] Color getEmptyTileColor() const;
  [[nodiscard]] Color getTextColor(int value) const;

  // Engine Components
  Engine::Context m_context;
  Engine::Window m_window;
  Engine::Renderer m_renderer;
  Engine::Font m_font; // Standard Tile Font (Size 40?)

  // Specific Fonts for UI
  Engine::Font m_fontTitle;            // Size 80
  Engine::Font m_fontSmall;            // Size 18 (Labels)
  Engine::Font m_fontMedium;           // Size 30 (Score Values)
  Engine::Font m_fontTiny;             // Size 20 (Compact Buttons)
  InputManager m_inputManager;         // Added
  AnimationManager m_animationManager; // Added
  Engine::SoundManager m_soundManager;
  std::set<std::pair<int, int>>
      m_hiddenTiles; // Tiles currently animating (do not render static)

  // Core Components
  Core::Grid m_grid;
  Core::GameLogic m_logic;

  // State
  bool m_isRunning;
  GameState m_state;
  GameState m_previousState; // Added for navigation
  int m_menuSelection;       // Reused for all menus

  // Settings
  bool m_darkSkin; // True = Dark Mode
  bool m_soundOn;  // True = Sound Enabled

  // Constants
  static constexpr int WINDOW_WIDTH = 600;
  static constexpr int WINDOW_HEIGHT = 800;
  static constexpr int TILE_SIZE = 120;
  static constexpr int GRID_PADDING = 20;
  static constexpr int GRID_OFFSET_X = 50;
  static constexpr int GRID_OFFSET_Y = 50;

  // Achievements State
  std::vector<bool> m_unlockedAchievements;
  bool m_showAchievementPopup;
  int m_popupAchievementIndex;
  float m_popupTimer;
  void checkAchievements();
};

} // namespace Game
