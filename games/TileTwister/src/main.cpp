#include "game/Game.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  try {
    Game::Game game;
    game.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
