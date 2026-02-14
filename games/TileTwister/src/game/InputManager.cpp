#include "InputManager.hpp"

namespace Game {

Action InputManager::pollAction(int &mouseX, int &mouseY, bool &mouseClicked) {
  SDL_Event e;
  mouseClicked = false; // Reset per frame/poll loop start

  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      return Action::Quit;
    } else if (e.type == SDL_KEYDOWN) {
      return translateKey(e.key.keysym.sym);
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
      if (e.button.button == SDL_BUTTON_LEFT) {
        mouseX = e.button.x;
        mouseY = e.button.y;
        mouseClicked = true;
        return Action::None; // Or Action::Click if we defined it, but logic
                             // will check bool
      }
    }
  }
  return Action::None;
}

Action InputManager::translateKey(SDL_Keycode key) {
  switch (key) {
  case SDLK_UP:
    return Action::Up;
  case SDLK_w:
    return Action::Up; // WASD Support added for free!

  case SDLK_DOWN:
    return Action::Down;
  case SDLK_s:
    return Action::Down;

  case SDLK_LEFT:
    return Action::Left;
  case SDLK_a:
    return Action::Left;

  case SDLK_RIGHT:
    return Action::Right;
  case SDLK_d:
    return Action::Right;

  case SDLK_ESCAPE:
    return Action::Back;
  case SDLK_r:
    return Action::Restart;
  case SDLK_RETURN:
  case SDLK_SPACE:         // Add SPACE as well
    return Action::Select; // Was Confirm
  case SDLK_BACKSPACE:
    return Action::Back;

  default:
    return Action::None;
  }
}

} // namespace Game
