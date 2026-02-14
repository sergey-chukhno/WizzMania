#include "core/states/MenuState.h"
#include "core/FontManager.h"
#include "core/Game.h"
#include "core/states/PlayingState.h"
#include "core/states/SettingsState.h"
#include "ui/AnimatedText.h"
#include "ui/Button.h"
#include "ui/Starfield.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MenuState::MenuState(Game *game)
    : game_(game), buttonLabels_{"START", "SETTINGS", "QUIT"},
      titleFloatTime_(0.0f), titleColorTime_(0.0f), titleBaseY_(0.0f),
      titleBasePosition_(0.0f, 0.0f) {
  initializeUI();
}

MenuState::~MenuState() = default;

void MenuState::initializeUI() {
  // Use Display font for title
  const sf::Font &displayFont = FontManager::getDisplayFont();
  // Use Body font for buttons
  const sf::Font &bodyFont = FontManager::getBodyFont();

  // Initialize starfield with more stars for enhanced space background
  starfield_ = std::make_unique<Starfield>(
      250, // Increased from 200 for more space objects
      sf::Vector2u(game_->getWindowWidth(), game_->getWindowHeight()));

  // Initialize title with animated text
  titleText_ =
      std::make_unique<AnimatedText>(displayFont, "SPACE BRICK BREAKER",
                                     80 // Increased size for impact
      );
  titleText_->setFillColor(getTitleColorPink()); // Start with pink
  titleText_->setPulseRange(0.8f, 1.0f);
  titleText_->setPulseSpeed(1.0f);
  titleText_->setGlowEnabled(true);
  titleText_->setShadowEnabled(true);

  // Center title and store base position
  sf::FloatRect titleBounds = titleText_->getLocalBounds();
  titleText_->setOrigin(
      sf::Vector2f(titleBounds.size.x / 2.0f, titleBounds.size.y / 2.0f));
  titleBaseY_ = 150.0f;
  titleBasePosition_ = sf::Vector2f(
      static_cast<float>(game_->getWindowWidth()) / 2.0f, titleBaseY_);
  titleText_->setPosition(titleBasePosition_);

  // Create buttons
  float startY = 350.0f;
  float centerX = static_cast<float>(game_->getWindowWidth()) / 2.0f;

  for (size_t i = 0; i < buttonLabels_.size(); ++i) {
    float buttonY = startY + i * (BUTTON_HEIGHT + BUTTON_SPACING);

    auto button = std::make_unique<Button>(
        displayFont, buttonLabels_[i], sf::Vector2f(centerX, buttonY),
        sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));

    button->setColors(getButtonFillColor(), getButtonOutlineColor(),
                      getButtonTextColor());

    // Set click callbacks
    // Use queueStateChange to safely defer state changes until after event
    // handling
    if (i == 0) // START
    {
      button->setOnClick([this]() {
        std::cout << "Start button clicked" << std::endl;
        game_->queueStateChange(std::make_unique<PlayingState>(game_));
      });
    } else if (i == 1) // SETTINGS
    {
      button->setOnClick([this]() {
        std::cout << "Settings button clicked" << std::endl;
        game_->queueStateChange(std::make_unique<SettingsState>(game_));
      });
    } else if (i == 2) // QUIT
    {
      button->setOnClick([this]() {
        std::cout << "Quit button clicked" << std::endl;
        game_->getWindow().close();
      });
    }

    buttons_.push_back(std::move(button));
  }
}

void MenuState::update(float deltaTime) {
  // Update starfield
  if (starfield_) {
    starfield_->update(deltaTime);
  }

  // Update title animations (floating and color)
  updateTitleAnimations(deltaTime);

  // Update title pulse animation
  if (titleText_) {
    titleText_->update(deltaTime);
  }

  // Update buttons
  for (auto &button : buttons_) {
    button->update(deltaTime);
  }
}

void MenuState::render(sf::RenderWindow &window) {
  // Render starfield first (background)
  if (starfield_) {
    starfield_->render(window);
  }

  // Render title
  if (titleText_) {
    titleText_->render(window);
  }

  // Render buttons
  for (const auto &button : buttons_) {
    button->render(window);
  }
}

void MenuState::handleEvent(const sf::Event &event) {
  // Handle keyboard navigation
  if (auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    if (keyPressed->code == sf::Keyboard::Key::Enter ||
        keyPressed->code == sf::Keyboard::Key::Space) {
      // Activate first button (START) - simulate click by triggering state
      // change
      game_->queueStateChange(std::make_unique<PlayingState>(game_));
    } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
      // Quit game
      game_->getWindow().close();
    }
  }

  // Pass events to buttons (they handle mouse clicks)
  for (auto &button : buttons_) {
    button->handleEvent(event, game_->getWindow());
  }
}

void MenuState::onEnter() { std::cout << "Entered MenuState" << std::endl; }

void MenuState::onExit() { std::cout << "Exited MenuState" << std::endl; }

void MenuState::updateTitleAnimations(float deltaTime) {
  if (!titleText_)
    return;

  // Update floating animation time
  titleFloatTime_ += deltaTime;

  // Update color animation time
  titleColorTime_ += deltaTime;

  // Calculate floating offset (vertical sine wave)
  float floatOffset = std::sin(TITLE_FLOAT_SPEED * 2.0f *
                               static_cast<float>(M_PI) * titleFloatTime_) *
                      TITLE_FLOAT_AMPLITUDE;

  // Update title position with floating offset
  sf::Vector2f newPosition = titleBasePosition_;
  newPosition.y = titleBaseY_ + floatOffset;
  titleText_->setPosition(newPosition);

  // Cycle through colors: Pink -> Cyan -> Purple -> Green -> Yellow -> Pink
  const int colorCount = 5;
  float colorCycle = std::fmod(titleColorTime_ * TITLE_COLOR_CHANGE_SPEED,
                               static_cast<float>(colorCount));
  int colorIndex1 = static_cast<int>(colorCycle) % colorCount;
  int colorIndex2 = (colorIndex1 + 1) % colorCount;
  float t = colorCycle - static_cast<float>(colorIndex1);

  // Get colors based on index
  sf::Color color1, color2;
  switch (colorIndex1) {
  case 0:
    color1 = getTitleColorPink();
    break;
  case 1:
    color1 = getTitleColorCyan();
    break;
  case 2:
    color1 = getTitleColorPurple();
    break;
  case 3:
    color1 = getTitleColorGreen();
    break;
  case 4:
    color1 = getTitleColorYellow();
    break;
  default:
    color1 = getTitleColorPink();
    break;
  }
  switch (colorIndex2) {
  case 0:
    color2 = getTitleColorPink();
    break;
  case 1:
    color2 = getTitleColorCyan();
    break;
  case 2:
    color2 = getTitleColorPurple();
    break;
  case 3:
    color2 = getTitleColorGreen();
    break;
  case 4:
    color2 = getTitleColorYellow();
    break;
  default:
    color2 = getTitleColorPink();
    break;
  }

  // Interpolate between colors
  sf::Color currentColor = lerpColor(color1, color2, t);
  titleText_->setFillColor(currentColor);
}

sf::Color MenuState::lerpColor(const sf::Color &color1, const sf::Color &color2,
                               float t) {
  // Clamp t to [0, 1]
  t = std::max(0.0f, std::min(1.0f, t));

  // Linear interpolation for each color component
  unsigned char r =
      static_cast<unsigned char>(color1.r + (color2.r - color1.r) * t);
  unsigned char g =
      static_cast<unsigned char>(color1.g + (color2.g - color1.g) * t);
  unsigned char b =
      static_cast<unsigned char>(color1.b + (color2.b - color1.b) * t);

  return sf::Color(r, g, b);
}
