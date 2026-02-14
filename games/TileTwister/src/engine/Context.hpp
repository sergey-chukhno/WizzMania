#pragma once
#include <SDL.h>
#include <stdexcept>
#include <string>

#include <SDL_image.h>
#include <SDL_ttf.h>

namespace Engine {

/**
 * @brief RAII wrapper for SDL Global State (Init/Quit)
 */
class Context {
public:
  Context() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      throw std::runtime_error("SDL_Init failed: " +
                               std::string(SDL_GetError()));
    }
    if (TTF_Init() == -1) {
      throw std::runtime_error("TTF_Init failed: " +
                               std::string(TTF_GetError()));
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      throw std::runtime_error("IMG_Init failed: " +
                               std::string(IMG_GetError()));
    }
  }

  ~Context() {
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
  }

  // Delete copy/move to ensure only one instance of global SDL state logic
  // exists (singleton-like behavior)
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  Context(Context &&) = delete;
  Context &operator=(Context &&) = delete;
};

} // namespace Engine
