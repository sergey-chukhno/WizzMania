#ifndef GAME_H
#define GAME_H

#include "core/GameState.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

// Forward declaration
class GameState;

/**
 * @brief Main game class that manages the window and game states.
 *
 * The Game class is responsible for:
 * - Creating and managing the SFML window
 * - Managing the game state stack
 * - Running the main game loop
 * - Handling state transitions (push, pop, change)
 * - Calculating delta time for frame-independent updates
 */
class Game {
public:
  /**
   * @brief Constructs a Game instance.
   *
   * Initializes the window with default settings and prepares
   * the game state stack.
   */
  Game();

  /**
   * @brief Destructor.
   */
  ~Game();

  /**
   * @brief Starts the main game loop.
   *
   * This method runs until the game is closed. It handles events,
   * updates the current state, and renders the current state.
   */
  void run();

  /**
   * @brief Pushes a new state onto the state stack.
   *
   * This is used for overlay states like Pause, which should
   * return to the previous state when closed.
   *
   * @param state Unique pointer to the new state
   */
  void pushState(std::unique_ptr<GameState> state);

  /**
   * @brief Pops the current state from the state stack.
   *
   * Returns to the previous state. Used for resuming from pause.
   */
  void popState();

  /**
   * @brief Replaces the current state with a new one.
   *
   * This clears the state stack and sets the new state as the
   * only state. Used for menu transitions.
   *
   * @param state Unique pointer to the new state
   */
  void changeState(std::unique_ptr<GameState> state);

  /**
   * @brief Queues a state change to be executed after event handling.
   *
   * This is safe to call from within event handlers (like button callbacks).
   * The state change will be processed at the end of the current frame.
   *
   * @param state Unique pointer to the new state
   */
  void queueStateChange(std::unique_ptr<GameState> state);

  /**
   * @brief Gets a pointer to the current state.
   * @return Pointer to the current state, or nullptr if no state exists
   */
  GameState *getCurrentState() const;

  /**
   * @brief Checks if the game is running.
   * @return True if the game is running, false otherwise
   */
  bool isRunning() const { return running_; }

  /**
   * @brief Gets the render window.
   * @return Reference to the render window
   */
  sf::RenderWindow &getWindow() { return window_; }

  /**
   * @brief Gets the window width.
   * @return Window width in pixels
   */
  unsigned int getWindowWidth() const { return WINDOW_WIDTH; }

  /**
   * @brief Gets the window height.
   * @return Window height in pixels
   */
  unsigned int getWindowHeight() const { return WINDOW_HEIGHT; }

  // Cyberpunk Color Palette (public for use by other classes)
  static const sf::Color NEON_PINK;   // #ff006e
  static const sf::Color NEON_CYAN;   // #00d9ff
  static const sf::Color NEON_PURPLE; // #9d4edd
  static const sf::Color NEON_GREEN;  // #06ffa5
  static const sf::Color BG_DARK;     // #0a0a1a

private:
  // Window constants
  static constexpr unsigned int WINDOW_WIDTH = 1280;
  static constexpr unsigned int WINDOW_HEIGHT = 720;
  static constexpr const char *WINDOW_TITLE = "Cyberpunk Cannon Shooter";

  // SFML window
  sf::RenderWindow window_;

  // State stack
  std::vector<std::unique_ptr<GameState>> stateStack_;

  // Pending state changes (processed after event handling)
  std::unique_ptr<GameState> pendingStateChange_;

  // Game loop control
  bool running_;
  sf::Clock clock_; // For delta time calculation

  // Fade transition
  float fadeAlpha_;    // Current fade alpha (0.0 to 1.0)
  float fadeSpeed_;    // Fade speed (alpha per second)
  bool isFading_;      // True if currently fading
  bool fadeDirection_; // True = fading in (0 to 1), False = fading out (1 to 0)
  sf::RectangleShape fadeOverlay_; // Fade overlay rectangle

  // Fade constants
  static constexpr float FADE_SPEED = 2.0f;    // Alpha units per second
  static constexpr float FADE_DURATION = 0.3f; // Seconds for full fade

  // Audio UI
  sf::RectangleShape soundButton_;
  sf::Text soundButtonText_;

  /**
   * @brief Initializes the sound button UI.
   */
  void initializeSoundButton();

  /**
   * @brief Updates the sound button visual state.
   */
  void updateSoundButton();

  /**
   * @brief Handles clicks on the sound button.
   * @param mousePos Mouse position
   * @return True if button was clicked
   */
  bool handleSoundButtonClick(const sf::Vector2i &mousePos);

  /**
   * @brief Handles window events.
   *
   * Processes global window events like closing the window.
   */
  void handleWindowEvents();

  /**
   * @brief Updates the current state.
   * @param deltaTime Time elapsed since last frame (in seconds)
   */
  void update(float deltaTime);

  /**
   * @brief Updates fade transition.
   * @param deltaTime Time elapsed since last frame (in seconds)
   */
  void updateFade(float deltaTime);

  /**
   * @brief Starts a fade transition.
   * @param fadeIn True to fade in (black to transparent), false to fade out
   * (transparent to black)
   */
  void startFade(bool fadeIn);

  /**
   * @brief Renders the current state.
   */
  void render();
};

#endif // GAME_H
