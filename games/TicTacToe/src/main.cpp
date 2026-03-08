#include "core/Game.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  std::string username = "Guest";
  std::string roomId = "";
  char symbol = 'X';
  std::string opponent = "Unknown";

  std::string avatarPath = "";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.find("--user=") == 0) {
      username = arg.substr(7);
    } else if (arg.find("--room=") == 0) {
      roomId = arg.substr(7);
    } else if (arg.find("--symbol=") == 0) {
      symbol = arg.substr(9)[0];
    } else if (arg.find("--opponent=") == 0) {
      opponent = arg.substr(11);
    } else if (arg.find("--avatarPath=") == 0) {
      avatarPath = arg.substr(13);
    }
  }

  try {
    Game game(username, roomId, symbol, opponent, avatarPath);
    game.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
