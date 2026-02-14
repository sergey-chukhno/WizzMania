#ifndef GAMEOVERSTATE_H
#define GAMEOVERSTATE_H

#include "core/Game.h"
#include "core/GameState.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Forward declaration
class Game;

/**
 * @brief Game Over state with score display and Restart/Menu buttons.
 *
 * GameOverState is displayed when the game ends. It shows the final score
 * and allows the player to restart or return to the menu.
 */
class GameOverState : public GameState {
public:
  /**
   * @brief Constructs a GameOverState.
   * @param game Pointer to the Game instance for state transitions
   * @param score Final score to display
   * @param level Level reached
   * @param bricksDestroyed Number of bricks destroyed
   */
  explicit GameOverState(Game *game, int score = 0, int level = 1,
                         int bricksDestroyed = 0);

  /**
   * @brief Destructor.
   */
  ~GameOverState() override = default;

  void update(float deltaTime) override;
  void render(sf::RenderWindow &window) override;
  void handleEvent(const sf::Event &event) override;
  void onEnter() override;
  void onExit() override;

private:
  Game *game_;
  int score_;
  int level_;
  int bricksDestroyed_;

  // UI elements
  const sf::Font &font_; // Reference to font from FontManager
  const sf::Font &displayFont_;
  sf::Text titleText_;
  sf::Text scoreText_;
  sf::Text levelText_;
  sf::Text bricksDestroyedText_;
  std::vector<sf::RectangleShape> buttons_;
  std::vector<sf::Text> buttonTexts_;
  std::vector<std::string> buttonLabels_;

  // Title animation state
  float titleFloatTime_;           // Time accumulator for floating animation
  float titleColorTime_;           // Time accumulator for color animation
  float titleBaseY_;               // Base Y position for floating
  sf::Vector2f titleBasePosition_; // Base position before floating offset

  // Statistics animation state
  float statisticsColorTime_; // Time accumulator for statistics color animation

  // Background space objects
  struct Planet {
    sf::CircleShape shape;
    float rotationAngle;
    float rotationSpeed;
    float orbitRadius;
    sf::Vector2f orbitCenter;
    float orbitAngle;
    float orbitSpeed;
    sf::Color color;
    bool hasRings;                         // Jupiter-like rings
    std::vector<sf::RectangleShape> rings; // Ring shapes
    bool isPulsarPlanet;                   // Pulsating glow effect
    sf::CircleShape glow;                  // Glow effect for pulsar planets
    float pulseTime;                       // Pulse animation time
    float pulseSpeed;                      // Pulse speed
  };
  struct Pulsar {
    sf::CircleShape core;
    sf::CircleShape glow;
    sf::Vector2f position;
    float pulseTime;
    float pulseSpeed;
    float baseRadius;
    float glowRadius;
    sf::Color color;
  };
  std::vector<Planet> planets_;
  std::vector<Pulsar> pulsars_;
  float backgroundAnimationTime_; // Time accumulator for background animations

  // Button colors
  static constexpr sf::Color BUTTON_COLOR =
      sf::Color(0, 217, 255, 150); // Cyan with transparency
  static constexpr sf::Color BUTTON_HOVER_COLOR =
      sf::Color(0, 217, 255, 200); // Cyan brighter
  static constexpr sf::Color TEXT_COLOR = sf::Color(255, 255, 255); // White
  static constexpr sf::Color TITLE_COLOR = sf::Color(255, 0, 110);  // Pink

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
  static constexpr float STATISTICS_COLOR_CHANGE_SPEED =
      1.0f; // Statistics color change speed (slightly slower)

  // Title colors (cyberpunk palette for color cycling)
  static sf::Color getTitleColorPink() { return Game::NEON_PINK; }
  static sf::Color getTitleColorCyan() { return Game::NEON_CYAN; }
  static sf::Color getTitleColorPurple() { return Game::NEON_PURPLE; }
  static sf::Color getTitleColorGreen() { return Game::NEON_GREEN; }
  static sf::Color getTitleColorYellow() {
    return sf::Color(255, 221, 0);
  } // Keep yellow for variety

  // Background constants
  static constexpr int PLANET_COUNT = 3;            // Number of planets
  static constexpr int PULSAR_COUNT = 4;            // Number of pulsars
  static constexpr float PLANET_MIN_RADIUS = 20.0f; // Minimum planet radius
  static constexpr float PLANET_MAX_RADIUS = 40.0f; // Maximum planet radius
  static constexpr float PLANET_MIN_ROTATION_SPEED =
      10.0f; // Degrees per second
  static constexpr float PLANET_MAX_ROTATION_SPEED = 30.0f;
  static constexpr float PLANET_MIN_ORBIT_SPEED = 5.0f; // Degrees per second
  static constexpr float PLANET_MAX_ORBIT_SPEED = 15.0f;
  static constexpr float PLANET_RING_COUNT = 3.0f; // Number of rings per planet
  static constexpr float PLANET_RING_WIDTH = 2.0f; // Ring thickness
  static constexpr float PLANET_RING_SPACING = 8.0f; // Space between rings
  static constexpr float PLANET_GLOW_MULTIPLIER =
      2.5f; // Glow radius for pulsar planets
  static constexpr float PLANET_PULSE_SPEED =
      1.5f; // Pulse speed for pulsar planets
  static constexpr float PULSAR_MIN_RADIUS = 3.0f;
  static constexpr float PULSAR_MAX_RADIUS = 6.0f;
  static constexpr float PULSAR_GLOW_MULTIPLIER =
      3.0f;                                         // Glow radius multiplier
  static constexpr float PULSAR_PULSE_SPEED = 2.0f; // Pulse cycles per second

  /**
   * @brief Initializes the UI elements.
   */
  void initializeUI();

  /**
   * @brief Handles mouse click on buttons.
   * @param mousePos Mouse position in window coordinates
   */
  void handleButtonClick(const sf::Vector2f &mousePos);

  /**
   * @brief Updates button hover states.
   * @param mousePos Mouse position in window coordinates
   */
  void updateButtonHover(const sf::Vector2f &mousePos);

  /**
   * @brief Gets the index of the button at the given position.
   * @param mousePos Mouse position in window coordinates
   * @return Button index, or -1 if no button at position
   */
  int getButtonAt(const sf::Vector2f &mousePos) const;

  /**
   * @brief Updates title floating and color animations.
   * @param deltaTime Time elapsed since last frame
   */
  void updateTitleAnimation(float deltaTime);

  /**
   * @brief Updates statistics color animations.
   * @param deltaTime Time elapsed since last frame
   */
  void updateStatisticsAnimations(float deltaTime);

  /**
   * @brief Interpolates between two colors.
   * @param color1 First color
   * @param color2 Second color
   * @param t Interpolation factor (0.0 to 1.0)
   * @return Interpolated color
   */
  static sf::Color lerpColor(const sf::Color &color1, const sf::Color &color2,
                             float t);

  /**
   * @brief Gets a color from the cyberpunk palette based on time offset.
   * @param timeOffset Offset in the color cycle (0.0 to colorCount)
   * @return Color from the palette
   */
  static sf::Color getColorFromPalette(float timeOffset);

  /**
   * @brief Initializes background space objects (planets and pulsars).
   */
  void initializeBackground();

  /**
   * @brief Updates background animations (planets and pulsars).
   * @param deltaTime Time elapsed since last frame
   */
  void updateBackground(float deltaTime);

  /**
   * @brief Renders background space objects.
   * @param window Render window
   */
  void renderBackground(sf::RenderWindow &window) const;

  /**
   * @brief Creates rings for a planet.
   * @param planet Planet to add rings to
   * @param planetRadius Radius of the planet
   */
  void createPlanetRings(Planet &planet, float planetRadius);
};

#endif // GAMEOVERSTATE_H
