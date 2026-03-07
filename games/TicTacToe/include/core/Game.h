#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

class State;

class Game {
public:
  Game(const std::string &username, const std::string &roomId, char symbol,
       const std::string &opponent, const std::string &avatarPath = "");
  ~Game();

  void run();

  const std::string &getUsername() const { return username_; }
  const std::string &getRoomId() const { return roomId_; }
  char getSymbol() const { return symbol_; }
  const std::string &getOpponent() const { return opponent_; }
  const std::string &getAvatarPath() const { return avatarPath_; }

  sf::RenderWindow &getWindow() { return window_; }
  void changeState(std::unique_ptr<State> state);

private:
  void processEvents();
  void update(sf::Time dt);
  void render();

  sf::RenderWindow window_;
  std::unique_ptr<State> currentState_;

  std::string username_;
  std::string roomId_;
  char symbol_;
  std::string opponent_;
  std::string avatarPath_;
};
