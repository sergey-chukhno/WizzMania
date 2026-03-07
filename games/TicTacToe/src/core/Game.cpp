#include "core/Game.h"
#include "core/states/PlayingState.h"
#include "core/states/State.h"

Game::Game(const std::string &username, const std::string &roomId, char symbol,
           const std::string &opponent, const std::string &avatarPath)
    : window_(sf::VideoMode({800, 610}), "WizzMania TicTacToe",
              sf::Style::Titlebar | sf::Style::Close),
      username_(username), roomId_(roomId), symbol_(symbol),
      opponent_(opponent), avatarPath_(avatarPath) {

  window_.setFramerateLimit(60);
  changeState(std::make_unique<PlayingState>(this));
}

Game::~Game() {}

void Game::run() {
  sf::Clock clock;
  while (window_.isOpen()) {
    sf::Time dt = clock.restart();
    processEvents();
    update(dt);
    render();
  }
}

void Game::changeState(std::unique_ptr<State> state) {
  if (currentState_) {
    currentState_->onExit();
  }
  currentState_ = std::move(state);
  if (currentState_) {
    currentState_->onEnter();
  }
}

void Game::processEvents() {
  while (const std::optional<sf::Event> event = window_.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      window_.close();
    }
    if (currentState_) {
      currentState_->processEvent(*event);
    }
  }
}

void Game::update(sf::Time dt) {
  if (currentState_) {
    currentState_->update(dt);
  }
}

void Game::render() {
  window_.clear(sf::Color(10, 15, 25)); // Dark Cyberpunk background
  if (currentState_) {
    currentState_->render(window_);
  }
  window_.display();
}
