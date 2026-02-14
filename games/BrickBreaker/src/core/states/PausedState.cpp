#include "core/states/PausedState.h"
#include "core/Game.h"
#include "core/states/MenuState.h"
#include "core/FontManager.h"
#include <iostream>
#include <memory>

PausedState::PausedState(Game* game)
    : game_(game)
    , font_(FontManager::getDefaultFont())
    , titleText_(font_, "PAUSED", 64)
    , buttonLabels_{"RESUME", "QUIT TO MENU"}
{
    initializeUI();
}

void PausedState::initializeUI()
{
    // Title
    titleText_.setFillColor(TITLE_COLOR);
    titleText_.setStyle(sf::Text::Bold);
    
    // Center title
    // SFML 3.0: Rect uses .size (Vector2f) instead of .width/.height
    sf::FloatRect titleBounds = titleText_.getLocalBounds();
    titleText_.setOrigin(sf::Vector2f(titleBounds.size.x / 2.0f, titleBounds.size.y / 2.0f));
    titleText_.setPosition(sf::Vector2f(
        static_cast<float>(game_->getWindowWidth()) / 2.0f,
        250.0f
    ));

    // Create buttons
    float startY = 400.0f;
    float centerX = static_cast<float>(game_->getWindowWidth()) / 2.0f;

    for (size_t i = 0; i < buttonLabels_.size(); ++i)
    {
        // Create button rectangle
        sf::RectangleShape button(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
        button.setFillColor(BUTTON_COLOR);
        button.setOutlineColor(sf::Color(0, 217, 255)); // Cyan outline
        button.setOutlineThickness(2.0f);
        // SFML 3.0: setOrigin and setPosition take Vector2f
        button.setOrigin(sf::Vector2f(BUTTON_WIDTH / 2.0f, BUTTON_HEIGHT / 2.0f));
        button.setPosition(sf::Vector2f(centerX, startY + i * (BUTTON_HEIGHT + BUTTON_SPACING)));
        buttons_.push_back(button);

        // Create button text
        // SFML 3.0: Text requires font in constructor
        sf::Text text(font_, buttonLabels_[i], 24);
        text.setFillColor(TEXT_COLOR);
        text.setStyle(sf::Text::Bold);
        
        // Center text on button
        // SFML 3.0: Rect uses .size (Vector2f) instead of .width/.height
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
        text.setPosition(sf::Vector2f(centerX, startY + i * (BUTTON_HEIGHT + BUTTON_SPACING)));
        buttonTexts_.push_back(text);
    }
}

void PausedState::update(float deltaTime)
{
    // Suppress unused parameter warning
    (void)deltaTime;
    
    // Update button hover states based on mouse position
    sf::Vector2i mousePixelPos = sf::Mouse::getPosition(game_->getWindow());
    sf::Vector2f mousePos = game_->getWindow().mapPixelToCoords(mousePixelPos);
    updateButtonHover(mousePos);
}

void PausedState::render(sf::RenderWindow& window)
{
    // Draw semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(
        static_cast<float>(game_->getWindowWidth()),
        static_cast<float>(game_->getWindowHeight())
    ));
    overlay.setFillColor(sf::Color(0, 0, 0, 150)); // Dark overlay
    window.draw(overlay);

    // Draw title
    window.draw(titleText_);

    // Draw buttons
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        window.draw(buttons_[i]);
        window.draw(buttonTexts_[i]);
    }
}

void PausedState::handleEvent(const sf::Event& event)
{
    if (auto* mouseButton = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButton->button == sf::Mouse::Button::Left)
        {
            sf::Vector2i mousePixelPos = sf::Mouse::getPosition(game_->getWindow());
            sf::Vector2f mousePos = game_->getWindow().mapPixelToCoords(mousePixelPos);
            handleButtonClick(mousePos);
        }
    }

    // Keyboard navigation
    if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        // P key or ESC: Resume
        if (keyPressed->code == sf::Keyboard::Key::P || keyPressed->code == sf::Keyboard::Key::Escape)
        {
            game_->popState(); // Resume by popping pause state
        }
        // Enter: Resume (first button)
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
        {
            game_->popState(); // Resume
        }
    }
}

void PausedState::onEnter()
{
    std::cout << "Entered PausedState" << std::endl;
}

void PausedState::onExit()
{
    std::cout << "Exited PausedState" << std::endl;
}

void PausedState::handleButtonClick(const sf::Vector2f& mousePos)
{
    int buttonIndex = getButtonAt(mousePos);
    if (buttonIndex == -1)
    {
        return;
    }

    switch (buttonIndex)
    {
        case 0: // RESUME
            std::cout << "Resume button clicked" << std::endl;
            game_->popState(); // Pop pause state to resume
            break;

        case 1: // QUIT TO MENU
            std::cout << "Quit to menu button clicked" << std::endl;
            // changeState clears the entire stack, so we don't need to pop first
            game_->queueStateChange(std::make_unique<MenuState>(game_));
            break;

        default:
            break;
    }
}

void PausedState::updateButtonHover(const sf::Vector2f& mousePos)
{
    int hoveredButton = getButtonAt(mousePos);
    
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        if (static_cast<int>(i) == hoveredButton)
        {
            buttons_[i].setFillColor(BUTTON_HOVER_COLOR);
        }
        else
        {
            buttons_[i].setFillColor(BUTTON_COLOR);
        }
    }
}

int PausedState::getButtonAt(const sf::Vector2f& mousePos) const
{
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        if (buttons_[i].getGlobalBounds().contains(mousePos))
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

