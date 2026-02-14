#pragma once
#include <SDL.h>

namespace Game {

enum class Action {
  None,
  Up,
  Down,
  Left,
  Right,
  Quit,
  Restart,
  Confirm,
  Back,
  Select
};

class InputManager {
public:
  InputManager() = default;
  ~InputManager() = default;

  // Polls for events and translates them into High-Level Actions.
  // Returns Action::None if no relevant event occurred this frame.
  // Polls for events and translates them into High-Level Actions.
  // Returns Action::None if no relevant event occurred this frame.
  // Also captures mouse state.
  Action pollAction(int &mouseX, int &mouseY, bool &mouseClicked);

private:
  Action translateKey(SDL_Keycode key);
};

} // namespace Game
