#ifndef PLAYINGSTATE_H
#define PLAYINGSTATE_H

#include "NativeSharedMemory.h"
#include "core/GameState.h"
#include "entities/Brick.h"
#include "entities/Cannon.h"
#include "entities/Projectile.h"
#include "managers/BlockManager.h"
#include "ui/Starfield.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declaration
class Game;

/**
 * @brief Playing state for active gameplay.
 *
 * PlayingState is the main gameplay state where the player controls
 * the cannon and shoots projectiles at blocks. Currently a placeholder
 * that will be implemented in later steps.
 */
class PlayingState : public GameState {
public:
  /**
   * @brief Constructs a PlayingState.
   * @param game Pointer to the Game instance for state transitions
   */
  explicit PlayingState(Game *game);

  /**
   * @brief Destructor.
   */
  ~PlayingState() override = default;

  void update(float deltaTime) override;
  void render(sf::RenderWindow &window) override;
  void handleEvent(const sf::Event &event) override;
  void onEnter() override;
  void onExit() override;

private:
  Game *game_;
  const sf::Font &font_; // Reference to font from FontManager

  // Background
  std::unique_ptr<Starfield> starfield_; // Starfield background

  // IPC
  std::unique_ptr<wizz::NativeSharedMemory> sharedMemory_;

  // Cannon
  std::unique_ptr<Cannon> cannon_;

  // Projectile pool
  ProjectilePool projectilePool_;

  // Block manager (wave-based spawning and descent)
  std::unique_ptr<BlockManager> blockManager_;

  // Track which bricks each projectile has recently hit (to prevent rapid
  // re-hits) Maps projectile pointer to a set of brick pointers it has hit
  std::unordered_map<Projectile *, std::vector<Brick *>> projectileHitBricks_;

  // Game state
  int currentLevel_;
  int score_;
  int displayedScore_;  // Displayed score (for animated count-up effect)
  int bricksDestroyed_; // Number of bricks destroyed

  // High score system
  int highScore_;
  static constexpr const char *HIGH_SCORE_FILE = "highscore.txt";

  // HUD display elements
  sf::Text scoreText_;
  sf::Text levelText_;
  sf::Text highScoreText_;

  // HUD animation state
  float hudAnimationTime_;       // Time accumulator for animations
  float scoreGlowIntensity_;     // Glow intensity for score (0.7-1.0)
  float levelGlowIntensity_;     // Glow intensity for level (0.7-1.0)
  float highScoreGlowIntensity_; // Glow intensity for high score (0.7-1.0)
  float levelChangeFlash_;       // Flash effect when level changes (0.0-1.0)
  float highScoreFlash_; // Flash effect when high score updates (0.0-1.0)

  // Score calculation constants
  static constexpr int BASE_SCORE_PER_BRICK = 10;
  static constexpr float LEVEL_MULTIPLIER_BASE = 1.0f;
  static constexpr float LEVEL_MULTIPLIER_STEP = 0.2f;
  static constexpr float HEALTH_MULTIPLIER_BASE = 1.0f;
  static constexpr float HEALTH_MULTIPLIER_STEP = 0.5f;

  // Animated score constants
  static constexpr float SCORE_ANIMATION_SPEED =
      500.0f; // Points per second for count-up

  // HUD layout constants
  static constexpr float HUD_LEFT_MARGIN = 20.0f;
  static constexpr float HUD_TOP_MARGIN = 20.0f;
  static constexpr float HUD_RIGHT_MARGIN = 200.0f;
  static constexpr float HUD_TEXT_SPACING =
      40.0f; // Space between score and level

  // HUD color helper functions (cyberpunk palette)
  static sf::Color getHUDScoreColor() {
    return sf::Color(0, 217, 255);
  } // Cyan #00d9ff
  static sf::Color getHUDLevelColor() {
    return sf::Color(0, 217, 255);
  } // Cyan #00d9ff
  static sf::Color getHUDHighScoreColor() {
    return sf::Color(255, 0, 110);
  } // Pink #ff006e

  // HUD font sizes
  static constexpr unsigned int HUD_SCORE_FONT_SIZE = 28;
  static constexpr unsigned int HUD_LEVEL_FONT_SIZE = 22;
  static constexpr unsigned int HUD_HIGH_SCORE_FONT_SIZE = 22;

  // HUD glow effect constants (minimal glow for better text readability)
  static constexpr int HUD_GLOW_LAYERS = 2; // Further reduced layers (was 3)
  static constexpr float HUD_GLOW_SCALE_STEP =
      0.05f; // Smaller scale increment (was 0.08f)
  static constexpr unsigned char HUD_GLOW_ALPHA_BASE =
      40; // Much reduced base alpha (was 80)
  static constexpr unsigned char HUD_GLOW_ALPHA_DECREMENT =
      15; // Alpha decrease per layer (was 20)
  static constexpr float HUD_GLOW_PULSE_SPEED =
      1.5f; // Slower pulse speed (was 2.0f)
  static constexpr float HUD_GLOW_INTENSITY_MIN =
      0.4f; // Lower minimum glow intensity (was 0.6f)
  static constexpr float HUD_GLOW_INTENSITY_MAX =
      0.7f; // Lower maximum glow intensity (was 0.9f)

  // HUD flash effect constants
  static constexpr float HUD_FLASH_DURATION = 0.8f; // Flash duration in seconds
  static constexpr float HUD_FLASH_FADE_SPEED = 2.0f; // Flash fade speed

  // Explosion particle system
  struct ExplosionParticle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifetime;
    float maxLifetime;
    float size;
  };
  std::deque<ExplosionParticle> explosionParticles_;

  // Collision detection constants
  static constexpr float COLLISION_OFFSET =
      8.0f; // Offset to prevent sticking (increased to prevent re-collisions)

  /**
   * @brief Checks collisions between projectiles and bricks.
   */
  void checkProjectileBrickCollisions();

  /**
   * @brief Performs AABB collision detection between two rectangles.
   * @param rect1 First rectangle
   * @param rect2 Second rectangle
   * @return True if rectangles intersect, false otherwise
   */
  bool checkAABBCollision(const sf::FloatRect &rect1,
                          const sf::FloatRect &rect2) const;

  /**
   * @brief Determines collision side and bounces projectile.
   * @param projectile Projectile to bounce
   * @param brickBounds Brick bounding rectangle
   */
  void bounceProjectileOffBrick(Projectile *projectile,
                                const sf::FloatRect &brickBounds);

  /**
   * @brief Creates an explosion effect at the specified position.
   * @param position Explosion position
   * @param color Explosion color (from brick)
   */
  void createExplosion(const sf::Vector2f &position, const sf::Color &color);

  /**
   * @brief Updates explosion particles.
   * @param deltaTime Time elapsed since last frame
   */
  void updateExplosionParticles(float deltaTime);

  /**
   * @brief Renders explosion particles.
   * @param window Render window
   */
  void renderExplosionParticles(sf::RenderWindow &window) const;

  /**
   * @brief Calculates score for destroying a brick.
   * @param level Current level
   * @param brickMaxHealth Maximum health of the brick
   * @return Score points awarded
   */
  int calculateScore(int level, int brickMaxHealth) const;

  /**
   * @brief Adds score to total score.
   * @param points Points to add
   */
  void addScore(int points);

  /**
   * @brief Updates animated score display.
   * @param deltaTime Time elapsed since last frame
   */
  void updateScoreDisplay(float deltaTime);

  /**
   * @brief Initializes score display text.
   */
  void initializeScoreDisplay();

  /**
   * @brief Initializes level display text.
   */
  void initializeLevelDisplay();

  /**
   * @brief Initializes high score display text.
   */
  void initializeHighScoreDisplay();

  /**
   * @brief Updates level display text.
   */
  void updateLevelDisplay();

  /**
   * @brief Updates high score display text.
   */
  void updateHighScoreDisplay();

  /**
   * @brief Updates HUD animations (glow effects, flashes).
   * @param deltaTime Time elapsed since last frame
   */
  void updateHUDAnimations(float deltaTime);

  /**
   * @brief Renders text with strong cyberpunk glow effect.
   * @param window Render window
   * @param text Text to render
   * @param baseColor Base color of the text
   * @param glowIntensity Glow intensity multiplier (0.7-1.0)
   * @param flashAlpha Flash alpha multiplier (0.0-1.0, for flash effects)
   */
  void renderTextWithGlow(sf::RenderWindow &window, const sf::Text &text,
                          const sf::Color &baseColor, float glowIntensity,
                          float flashAlpha = 1.0f) const;

  /**
   * @brief Triggers level change flash effect.
   */
  void triggerLevelChangeFlash();

  /**
   * @brief Triggers high score flash effect.
   */
  void triggerHighScoreFlash();

  /**
   * @brief Loads high score from file.
   */
  void loadHighScore();

  /**
   * @brief Saves high score to file.
   */
  void saveHighScore();

  /**
   * @brief Gets the current score.
   * @return Current score
   */
  int getScore() const { return score_; }

  /**
   * @brief Gets the current level.
   * @return Current level
   */
  int getCurrentLevel() const { return currentLevel_; }

  /**
   * @brief Gets the number of bricks destroyed.
   * @return Number of bricks destroyed
   */
  int getBricksDestroyed() const { return bricksDestroyed_; }
};

#endif // PLAYINGSTATE_H
