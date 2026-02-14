#include "core/states/SettingsState.h"
#include "core/AudioManager.h"
#include "core/FontManager.h"
#include "core/Game.h"
#include "core/states/MenuState.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

// Note: SettingsState no longer stores previousState_ since we use
// changeState() which clears the stack. The back button creates a new
// MenuState.

SettingsState::SettingsState(Game *game)
    : game_(game), font_(FontManager::getBodyFont()),
      displayFont_(FontManager::getDisplayFont()),
      titleText_(displayFont_, "SETTINGS", 64),
      volumeLabel_(displayFont_, "MASTER VOLUME", 32),
      volumeValueText_(font_, "100%", 32),
      volumeMinusText_(font_, "-", 30), // Reduced font size
      volumePlusText_(font_, "+", 30),  // Reduced font size
      controlsTitle_(displayFont_, "CONTROLS", 32),
      controlsInfo_(font_,
                    "SPACE / CLICK : SHOOT\n"
                    "A / LEFT      : MOVE LEFT\n"
                    "D / RIGHT     : MOVE RIGHT\n"
                    "ESC           : PAUSE / BACK",
                    24),
      backButtonText_(displayFont_, "BACK", 24) {
  initializeUI();
}

void SettingsState::initializeUI() {
  float centerX = static_cast<float>(game_->getWindowWidth()) / 2.0f;

  // Title
  titleText_.setFillColor(TITLE_COLOR);
  titleText_.setStyle(sf::Text::Bold);
  sf::FloatRect titleBounds = titleText_.getLocalBounds();
  titleText_.setOrigin(
      sf::Vector2f(titleBounds.size.x / 2.0f, titleBounds.size.y / 2.0f));
  titleText_.setPosition(sf::Vector2f(centerX, 100.0f));

  // --- Volume Control ---
  float volumeY = 220.0f;

  // Label
  volumeLabel_.setFillColor(sf::Color::Cyan);
  sf::FloatRect volLabelBounds = volumeLabel_.getLocalBounds();
  volumeLabel_.setOrigin(
      sf::Vector2f(volLabelBounds.size.x / 2.0f, volLabelBounds.size.y / 2.0f));
  volumeLabel_.setPosition(sf::Vector2f(centerX, volumeY));

  // Buttons and Value
  float controlY = volumeY + 60.0f;
  float spacing = 80.0f;

  // Minus Button
  volumeMinusButton_.setSize(
      sf::Vector2f(SMALL_BUTTON_SIZE, SMALL_BUTTON_SIZE));
  volumeMinusButton_.setFillColor(BUTTON_COLOR);
  volumeMinusButton_.setOutlineColor(sf::Color::Cyan);
  volumeMinusButton_.setOutlineThickness(2.0f);
  volumeMinusButton_.setOrigin(
      sf::Vector2f(SMALL_BUTTON_SIZE / 2.0f, SMALL_BUTTON_SIZE / 2.0f));
  volumeMinusButton_.setPosition(sf::Vector2f(centerX - spacing, controlY));

  // Minus Text
  volumeMinusText_.setFillColor(TEXT_COLOR);
  sf::FloatRect minusBounds = volumeMinusText_.getLocalBounds();
  // Ensure we use the local bounds "top" and "left" to center correctly
  volumeMinusText_.setOrigin(
      sf::Vector2f(minusBounds.position.x + minusBounds.size.x / 2.0f,
                   minusBounds.position.y + minusBounds.size.y / 2.0f));
  volumeMinusText_.setPosition(sf::Vector2f(centerX - spacing, controlY));

  // Plus Button
  volumePlusButton_.setSize(sf::Vector2f(SMALL_BUTTON_SIZE, SMALL_BUTTON_SIZE));
  volumePlusButton_.setFillColor(BUTTON_COLOR);
  volumePlusButton_.setOutlineColor(sf::Color::Cyan);
  volumePlusButton_.setOutlineThickness(2.0f);
  volumePlusButton_.setOrigin(
      sf::Vector2f(SMALL_BUTTON_SIZE / 2.0f, SMALL_BUTTON_SIZE / 2.0f));
  volumePlusButton_.setPosition(sf::Vector2f(centerX + spacing, controlY));

  // Plus Text
  volumePlusText_.setFillColor(TEXT_COLOR);
  sf::FloatRect plusBounds = volumePlusText_.getLocalBounds();
  // Ensure we use the local bounds "top" and "left" to center correctly
  volumePlusText_.setOrigin(
      sf::Vector2f(plusBounds.position.x + plusBounds.size.x / 2.0f,
                   plusBounds.position.y + plusBounds.size.y / 2.0f));
  volumePlusText_.setPosition(sf::Vector2f(centerX + spacing, controlY));

  // Value Text (Initial update)
  updateVolumeDisplay();
  volumeValueText_.setFillColor(TEXT_COLOR);
  volumeValueText_.setPosition(sf::Vector2f(centerX, controlY));

  // --- Controls Info ---
  float controlsY = 400.0f;

  controlsTitle_.setFillColor(sf::Color::Cyan);
  sf::FloatRect ctrlTitleBounds = controlsTitle_.getLocalBounds();
  controlsTitle_.setOrigin(sf::Vector2f(ctrlTitleBounds.size.x / 2.0f,
                                        ctrlTitleBounds.size.y / 2.0f));
  controlsTitle_.setPosition(sf::Vector2f(centerX, controlsY));

  controlsInfo_.setFillColor(TEXT_COLOR);
  controlsInfo_.setLineSpacing(1.5f);
  sf::FloatRect infoBounds = controlsInfo_.getLocalBounds();
  controlsInfo_.setOrigin(
      sf::Vector2f(infoBounds.size.x / 2.0f, infoBounds.size.y / 2.0f));
  controlsInfo_.setPosition(sf::Vector2f(centerX, controlsY + 100.0f));

  // Back button
  float buttonY = 660.0f; // Moved down from 620.0f

  backButton_.setSize(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
  backButton_.setFillColor(BUTTON_COLOR);
  backButton_.setOutlineColor(sf::Color(0, 217, 255)); // Cyan outline
  backButton_.setOutlineThickness(2.0f);
  backButton_.setOrigin(
      sf::Vector2f(BUTTON_WIDTH / 2.0f, BUTTON_HEIGHT / 2.0f));
  backButton_.setPosition(sf::Vector2f(centerX, buttonY));

  backButtonText_.setFillColor(TEXT_COLOR);
  backButtonText_.setStyle(sf::Text::Bold);
  sf::FloatRect textBounds = backButtonText_.getLocalBounds();
  backButtonText_.setOrigin(
      sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
  backButtonText_.setPosition(sf::Vector2f(centerX, buttonY));
}

void SettingsState::updateVolumeDisplay() {
  int volume = static_cast<int>(AudioManager::getInstance().getGlobalVolume());
  volumeValueText_.setString(std::to_string(volume) + "%");

  sf::FloatRect valBounds = volumeValueText_.getLocalBounds();
  volumeValueText_.setOrigin(
      sf::Vector2f(valBounds.size.x / 2.0f, valBounds.size.y / 2.0f));
}

void SettingsState::update(float deltaTime) {
  (void)deltaTime;

  sf::Vector2i mousePixelPos = sf::Mouse::getPosition(game_->getWindow());
  sf::Vector2f mousePos = game_->getWindow().mapPixelToCoords(mousePixelPos);

  // Update hover states
  auto updateButtonColor = [&](sf::RectangleShape &btn) {
    if (btn.getGlobalBounds().contains(mousePos)) {
      btn.setFillColor(BUTTON_HOVER_COLOR);
    } else {
      btn.setFillColor(BUTTON_COLOR);
    }
  };

  updateButtonColor(backButton_);
  updateButtonColor(volumeMinusButton_);
  updateButtonColor(volumePlusButton_);
}

void SettingsState::render(sf::RenderWindow &window) {
  window.draw(titleText_);

  // Draw Volume Controls
  window.draw(volumeLabel_);
  window.draw(volumeMinusButton_);
  window.draw(volumeMinusText_);
  window.draw(volumePlusButton_);
  window.draw(volumePlusText_);
  window.draw(volumeValueText_);

  // Draw Controls Info
  window.draw(controlsTitle_);
  window.draw(controlsInfo_);

  // Back Button
  window.draw(backButton_);
  window.draw(backButtonText_);
}

void SettingsState::handleEvent(const sf::Event &event) {
  if (auto *mouseButton = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mouseButton->button == sf::Mouse::Button::Left) {
      sf::Vector2i mousePixelPos = sf::Mouse::getPosition(game_->getWindow());
      sf::Vector2f mousePos =
          game_->getWindow().mapPixelToCoords(mousePixelPos);

      if (backButton_.getGlobalBounds().contains(mousePos)) {
        handleBackButton();
      } else if (volumeMinusButton_.getGlobalBounds().contains(mousePos)) {
        float currentVol = AudioManager::getInstance().getGlobalVolume();
        float newVol = std::max(0.0f, currentVol - 10.0f);
        AudioManager::getInstance().setGlobalVolume(newVol);
        updateVolumeDisplay();
      } else if (volumePlusButton_.getGlobalBounds().contains(mousePos)) {
        float currentVol = AudioManager::getInstance().getGlobalVolume();
        float newVol = std::min(100.0f, currentVol + 10.0f);
        AudioManager::getInstance().setGlobalVolume(newVol);
        updateVolumeDisplay();
      }
    }
  }

  if (auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    if (keyPressed->code == sf::Keyboard::Key::Escape ||
        keyPressed->code == sf::Keyboard::Key::Backspace) {
      handleBackButton();
    }
  }
}

void SettingsState::onEnter() {
  std::cout << "Entered SettingsState" << std::endl;
  updateVolumeDisplay(); // Ensure volume is up to date when entering
}

void SettingsState::onExit() {
  std::cout << "Exited SettingsState" << std::endl;
}

void SettingsState::handleBackButton() {
  std::cout << "Back button clicked" << std::endl;
  game_->queueStateChange(std::make_unique<MenuState>(game_));
}
