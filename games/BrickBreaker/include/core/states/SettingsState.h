#ifndef SETTINGSSTATE_H
#define SETTINGSSTATE_H

#include "core/GameState.h"
#include <SFML/Graphics.hpp>
#include <memory>

// Forward declaration
class Game;
class GameState;

/**
 * @brief Settings state for game configuration.
 *
 * SettingsState is displayed when the player wants to change game settings.
 * The back button returns to the menu by creating a new MenuState.
 */
class SettingsState : public GameState {
public:
  /**
   * @brief Constructs a SettingsState.
   * @param game Pointer to the Game instance for state transitions
   */
  explicit SettingsState(Game *game);

  /**
   * @brief Destructor.
   */
  ~SettingsState() override = default;

  void update(float deltaTime) override;
  void render(sf::RenderWindow &window) override;
  void handleEvent(const sf::Event &event) override;
  void onEnter() override;
  void onExit() override;

private:
  Game *game_;

  // UI elements
  const sf::Font &font_; // Reference to font from FontManager
  const sf::Font &displayFont_;

  // Main Title
  sf::Text titleText_;

  // Volume Control Section
  sf::Text volumeLabel_;
  sf::Text volumeValueText_;
  sf::RectangleShape volumeMinusButton_;
  sf::Text volumeMinusText_;
  sf::RectangleShape volumePlusButton_;
  sf::Text volumePlusText_;

  // Controls Info Section
  sf::Text controlsTitle_;
  sf::Text controlsInfo_;

  // Navigation
  sf::RectangleShape backButton_;
  sf::Text backButtonText_;

  // Constants
  static constexpr float BUTTON_WIDTH = 200.0f;
  static constexpr float BUTTON_HEIGHT = 50.0f;
  static constexpr float SMALL_BUTTON_SIZE = 40.0f;
  static constexpr sf::Color BUTTON_COLOR = sf::Color(0, 30, 40, 200);
  static constexpr sf::Color BUTTON_HOVER_COLOR = sf::Color(0, 60, 80, 255);
  static constexpr sf::Color TEXT_COLOR = sf::Color::White;
  static constexpr sf::Color TITLE_COLOR = sf::Color(255, 0, 110); // Neon Pink

  // Helper to update volume text
  void updateVolumeDisplay();

  /**
   * @brief Initializes the UI elements.
   */
  void initializeUI();

  /**
   * @brief Handles back button click.
   */
  void handleBackButton();
};

#endif // SETTINGSSTATE_H
