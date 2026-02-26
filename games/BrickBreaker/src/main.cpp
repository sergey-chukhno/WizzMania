#include "core/FontManager.h"
#include "core/Game.h"
#include <iostream>

/**
 * @brief Main entry point for the Cyberpunk Cannon Shooter game.
 *
 * Creates a Game instance and runs the main game loop.
 */
int main(int argc, char *argv[]) {
  try {
    std::string username = "Guest";
    if (argc > 1) {
      username = argv[1];
    }
    Game game(username);
    game.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  // Explicitly cleanup resources
  FontManager::cleanup();

  return 0;
}
