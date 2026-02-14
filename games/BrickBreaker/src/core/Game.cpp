#include "core/Game.h"
#include "core/AudioManager.h"
#include "core/FontManager.h"
#include "core/states/MenuState.h"
#include <iostream>

// Initialize Cyberpunk Color Palette
const sf::Color Game::NEON_PINK(255, 0, 110);
const sf::Color Game::NEON_CYAN(0, 217, 255);
const sf::Color Game::NEON_PURPLE(157, 78, 221);
const sf::Color Game::NEON_GREEN(6, 255, 165);
const sf::Color Game::BG_DARK(10, 10, 26);

Game::Game()
    : window_(sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)),
              WINDOW_TITLE),
      running_(true), fadeAlpha_(1.0f) // Start fully faded (black screen)
      ,
      fadeSpeed_(FADE_SPEED), isFading_(true),
      fadeDirection_(true) // Fade in (black to transparent)
      ,
      soundButtonText_(FontManager::getBodyFont()) {
  window_.setFramerateLimit(60);

  // Initialize fade overlay
  fadeOverlay_.setSize(sf::Vector2f(static_cast<float>(WINDOW_WIDTH),
                                    static_cast<float>(WINDOW_HEIGHT)));
  fadeOverlay_.setFillColor(sf::Color(0, 0, 0, 255)); // Black
  fadeOverlay_.setPosition(sf::Vector2f(0, 0));

  // Start with MenuState
  changeState(std::make_unique<MenuState>(this));

  // Initialize Audio
  if (AudioManager::getInstance().loadMusic(
          "assets/audio/cyberpunk_theme.wav")) {
    AudioManager::getInstance().playMusic(true);
  }
  AudioManager::getInstance().loadSound("shoot",
                                        "assets/audio/laser_shoot.wav");

  // Initialize UI
  initializeSoundButton();

  std::cout << "Game initialized" << std::endl;
}

Game::~Game() {
  // Clean up states
  stateStack_.clear();
  std::cout << "Game destroyed" << std::endl;
}

void Game::run() {
  while (running_ && window_.isOpen()) {
    // Calculate delta time
    float deltaTime = clock_.restart().asSeconds();

    // Handle window events
    handleWindowEvents();

    // Process pending state changes (after event handling is complete)
    if (pendingStateChange_) {
      changeState(std::move(pendingStateChange_));
      pendingStateChange_.reset();
      // After state change, continue to update/render in the same frame
      // This ensures the new state is displayed immediately
    }

    // Update fade transition
    updateFade(deltaTime);

    // Update and render current state
    if (!stateStack_.empty()) {
      update(deltaTime);
      render();
    } else {
      // No states left, exit
      running_ = false;
    }
  }
}

void Game::pushState(std::unique_ptr<GameState> state) {
  if (!stateStack_.empty()) {
    // Notify current state that it's being paused
    stateStack_.back()->onExit();
  }

  // Push new state
  stateStack_.push_back(std::move(state));
  stateStack_.back()->onEnter();

  // Start fade in for overlay states (like pause)
  startFade(true);
}

void Game::popState() {
  if (!stateStack_.empty()) {
    // Notify current state that it's being exited
    stateStack_.back()->onExit();
    stateStack_.pop_back();

    // Notify new current state that it's being resumed
    if (!stateStack_.empty()) {
      stateStack_.back()->onEnter();
    }

    // Start fade in when resuming
    startFade(true);
  }
}

void Game::changeState(std::unique_ptr<GameState> state) {
  // Clear the entire state stack
  while (!stateStack_.empty()) {
    stateStack_.back()->onExit();
    stateStack_.pop_back();
  }

  // Set new state
  if (state) {
    stateStack_.push_back(std::move(state));
    stateStack_.back()->onEnter();
  }

  // Start fade in (fade from black to transparent)
  startFade(true);
}

void Game::queueStateChange(std::unique_ptr<GameState> state) {
  // Store the state change to be processed after event handling
  pendingStateChange_ = std::move(state);
}

GameState *Game::getCurrentState() const {
  if (stateStack_.empty()) {
    return nullptr;
  }
  return stateStack_.back().get();
}

void Game::handleWindowEvents() {
  // Handle global window events (like window close)
  while (std::optional<sf::Event> event = window_.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      window_.close();
      running_ = false;
      return;
    }

    // Pass event to current state
    if (!stateStack_.empty()) {
      stateStack_.back()->handleEvent(*event);
    }

    // Handle sound button click (global)
    if (event->is<sf::Event::MouseButtonPressed>()) {
      if (event->getIf<sf::Event::MouseButtonPressed>()->button ==
          sf::Mouse::Button::Left) {
        handleSoundButtonClick(sf::Mouse::getPosition(window_));
      }
    }
  }
}

void Game::update(float deltaTime) {
  if (!stateStack_.empty()) {
    stateStack_.back()->update(deltaTime);
  }
}

void Game::updateFade(float deltaTime) {
  if (!isFading_) {
    return;
  }

  if (fadeDirection_) // Fading in (black to transparent)
  {
    fadeAlpha_ -= fadeSpeed_ * deltaTime;
    if (fadeAlpha_ <= 0.0f) {
      fadeAlpha_ = 0.0f;
      isFading_ = false;
    }
  } else // Fading out (transparent to black)
  {
    fadeAlpha_ += fadeSpeed_ * deltaTime;
    if (fadeAlpha_ >= 1.0f) {
      fadeAlpha_ = 1.0f;
      isFading_ = false;
    }
  }
}

void Game::startFade(bool fadeIn) {
  isFading_ = true;
  fadeDirection_ = fadeIn;
  if (fadeIn) {
    fadeAlpha_ = 1.0f; // Start from black
  } else {
    fadeAlpha_ = 0.0f; // Start from transparent
  }
}

void Game::render() {
  window_.clear(BG_DARK); // Cyberpunk background color

  if (!stateStack_.empty()) {
    stateStack_.back()->render(window_);
  }

  // Render fade overlay if fading
  if (fadeAlpha_ > 0.0f) {
    unsigned char alpha = static_cast<unsigned char>(fadeAlpha_ * 255.0f);
    fadeOverlay_.setFillColor(sf::Color(0, 0, 0, alpha));
    window_.draw(fadeOverlay_);
    window_.draw(fadeOverlay_);
  }

  // Draw sound button on top of everything
  window_.draw(soundButton_);
  window_.draw(soundButtonText_);

  window_.display();
}
void Game::initializeSoundButton() {
  // Button background - small button in top-right corner
  soundButton_.setSize(sf::Vector2f(100.0f, 32.0f));
  soundButton_.setPosition(sf::Vector2f(WINDOW_WIDTH - 120.0f, 80.0f));
  soundButton_.setFillColor(sf::Color(10, 10, 26, 200)); // Dark fill
  soundButton_.setOutlineThickness(1.5f);

  // Button text
  soundButtonText_.setFont(FontManager::getDisplayFont());
  soundButtonText_.setCharacterSize(14);

  updateSoundButton();
}

void Game::updateSoundButton() {
  bool enabled = AudioManager::getInstance().isAudioEnabled();

  if (enabled) {
    soundButtonText_.setString("SOUND ON");
    soundButtonText_.setFillColor(NEON_CYAN);
    soundButton_.setOutlineColor(NEON_CYAN);
  } else {
    soundButtonText_.setString("SOUND OFF");
    soundButtonText_.setFillColor(NEON_PINK);
    soundButton_.setOutlineColor(NEON_PINK);
  }

  // Center text in button
  sf::FloatRect textBounds = soundButtonText_.getLocalBounds();
  sf::Vector2f btnPos = soundButton_.getPosition();
  sf::Vector2f btnSize = soundButton_.getSize();

  soundButtonText_.setPosition(
      sf::Vector2f(btnPos.x + (btnSize.x - textBounds.size.x) / 2.0f,
                   btnPos.y + (btnSize.y - textBounds.size.y) / 2.0f -
                       3.0f // Adjust for baseline
                   ));
}

bool Game::handleSoundButtonClick(const sf::Vector2i &mousePos) {
  sf::Vector2f worldPos = window_.mapPixelToCoords(mousePos);
  if (soundButton_.getGlobalBounds().contains(worldPos)) {
    AudioManager::getInstance().toggleAudio();
    updateSoundButton();
    return true;
  }
  return false;
}
