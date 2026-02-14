#ifndef MENUSTATE_H
#define MENUSTATE_H

#include "core/Game.h"
#include "core/GameState.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class Game;
class Button;
class Starfield;
class AnimatedText;

/**
 * @brief Menu state with Start, Settings, and Quit buttons.
 *
 * MenuState displays the main menu with buttons that allow the player to:
 * - Start the game (transitions to PlayingState)
 * - Open settings (transitions to SettingsState)
 * - Quit the game (exits application)
 *
 * Enhanced with cyberpunk aesthetics:
 * - Animated starfield background
 * - Glowing buttons with hover effects
 * - Pulsing title text
 */
class MenuState : public GameState {
public:
  /**
   * @brief Constructs a MenuState.
   * @param game Pointer to the Game instance for state transitions
   */
  explicit MenuState(Game *game);

  /**
   * @brief Destructor.
   */
  ~MenuState() override;

  void update(float deltaTime) override;
  void render(sf::RenderWindow &window) override;
  void handleEvent(const sf::Event &event) override;
  void onEnter() override;
  void onExit() override;

private:
  Game *game_;

  // UI components
  std::unique_ptr<Starfield> starfield_;
  std::unique_ptr<AnimatedText> titleText_;
  std::vector<std::unique_ptr<Button>> buttons_;
  std::vector<std::string> buttonLabels_;

  // Title animation state
  float titleFloatTime_;           // Time accumulator for floating animation
  float titleColorTime_;           // Time accumulator for color animation
  float titleBaseY_;               // Base Y position for floating
  sf::Vector2f titleBasePosition_; // Base position before floating offset

  // Button dimensions
  static constexpr float BUTTON_WIDTH = 300.0f;
  static constexpr float BUTTON_HEIGHT = 60.0f;
  static constexpr float BUTTON_SPACING = 20.0f;

  // Title animation constants
  static constexpr float TITLE_FLOAT_AMPLITUDE =
      15.0f; // Vertical float amplitude (pixels)
  static constexpr float TITLE_FLOAT_SPEED =
      0.8f; // Float speed (cycles per second) - slowed down
  static constexpr float TITLE_COLOR_CHANGE_SPEED = 1.2f; // Color change speed

  // Colors (cyberpunk palette for title flashing)
  static sf::Color getTitleColorPink() { return Game::NEON_PINK; }
  static sf::Color getTitleColorCyan() { return Game::NEON_CYAN; }
  static sf::Color getTitleColorPurple() { return Game::NEON_PURPLE; }
  static sf::Color getTitleColorGreen() { return Game::NEON_GREEN; }
  static sf::Color getTitleColorYellow() {
    return sf::Color(255, 221, 0);
  } // Keep yellow for variety

  // Button colors - using Game constants
  // Note: We can't use Game::NEON_CYAN directly in constexpr, so we'll use
  // static const or methods
  static sf::Color getButtonFillColor() {
    return sf::Color(0, 217, 255, 30);
  } // Transparent Cyan
  static sf::Color getButtonOutlineColor() { return Game::NEON_CYAN; }
  static sf::Color getButtonTextColor() { return sf::Color::White; }

  /**
   * @brief Initializes the UI elements.
   */
  void initializeUI();

  /**
   * @brief Updates title floating and color animations.
   * @param deltaTime Time elapsed since last frame
   */
  void updateTitleAnimations(float deltaTime);

  /**
   * @brief Interpolates between two colors.
   * @param color1 First color
   * @param color2 Second color
   * @param t Interpolation factor (0.0 to 1.0)
   * @return Interpolated color
   */
  static sf::Color lerpColor(const sf::Color &color1, const sf::Color &color2,
                             float t);
};

#endif // MENUSTATE_H
