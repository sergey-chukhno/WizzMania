#ifndef PAUSEDSTATE_H
#define PAUSEDSTATE_H

#include "core/GameState.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// Forward declaration
class Game;

/**
 * @brief Paused state with Resume and Quit buttons.
 * 
 * PausedState is displayed when the game is paused during gameplay.
 * It allows the player to resume the game or quit to the menu.
 */
class PausedState : public GameState
{
public:
    /**
     * @brief Constructs a PausedState.
     * @param game Pointer to the Game instance for state transitions
     */
    explicit PausedState(Game* game);

    /**
     * @brief Destructor.
     */
    ~PausedState() override = default;

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;
    void onEnter() override;
    void onExit() override;

private:
    Game* game_;

    // UI elements
    const sf::Font& font_;  // Reference to font from FontManager
    sf::Text titleText_;
    std::vector<sf::RectangleShape> buttons_;
    std::vector<sf::Text> buttonTexts_;
    std::vector<std::string> buttonLabels_;

    // Button colors
    static constexpr sf::Color BUTTON_COLOR = sf::Color(0, 217, 255, 150);      // Cyan with transparency
    static constexpr sf::Color BUTTON_HOVER_COLOR = sf::Color(0, 217, 255, 200); // Cyan brighter
    static constexpr sf::Color TEXT_COLOR = sf::Color(255, 255, 255);            // White
    static constexpr sf::Color TITLE_COLOR = sf::Color(255, 0, 110);             // Pink

    // Button dimensions
    static constexpr float BUTTON_WIDTH = 300.0f;
    static constexpr float BUTTON_HEIGHT = 60.0f;
    static constexpr float BUTTON_SPACING = 20.0f;

    /**
     * @brief Initializes the UI elements.
     */
    void initializeUI();

    /**
     * @brief Handles mouse click on buttons.
     * @param mousePos Mouse position in window coordinates
     */
    void handleButtonClick(const sf::Vector2f& mousePos);

    /**
     * @brief Updates button hover states.
     * @param mousePos Mouse position in window coordinates
     */
    void updateButtonHover(const sf::Vector2f& mousePos);

    /**
     * @brief Gets the index of the button at the given position.
     * @param mousePos Mouse position in window coordinates
     * @return Button index, or -1 if no button at position
     */
    int getButtonAt(const sf::Vector2f& mousePos) const;
};

#endif // PAUSEDSTATE_H

