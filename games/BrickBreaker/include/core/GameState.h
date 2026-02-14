#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <SFML/Graphics.hpp>

/**
 * @brief Base class for all game states.
 * 
 * GameState is an abstract base class that defines the interface for all
 * game states (Menu, Playing, Paused, GameOver, Settings). Each state
 * must implement the pure virtual methods: update, render, and handleEvent.
 * 
 * The onEnter and onExit methods are optional and can be overridden to
 * perform initialization and cleanup when states are entered or exited.
 */
class GameState
{
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~GameState() = default;

    /**
     * @brief Update the game state logic.
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Render the game state.
     * @param window Reference to the render window
     */
    virtual void render(sf::RenderWindow& window) = 0;

    /**
     * @brief Handle input events for this state.
     * @param event The SFML event to handle
     */
    virtual void handleEvent(const sf::Event& event) = 0;

    /**
     * @brief Called when this state is entered.
     * 
     * Override this method to perform initialization when the state becomes active.
     */
    virtual void onEnter() {}

    /**
     * @brief Called when this state is exited.
     * 
     * Override this method to perform cleanup when the state becomes inactive.
     */
    virtual void onExit() {}
};

#endif // GAMESTATE_H

