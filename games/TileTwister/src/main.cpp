#define SDL_MAIN_HANDLED
#include "game/Game.hpp"
#include <SDL2/SDL.h>
#include <iostream>

int main(int argc, char *argv[]) {
  std::string username = (argc > 1) ? argv[1] : "player";

  try {
    Game::Game game(username);
    game.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
