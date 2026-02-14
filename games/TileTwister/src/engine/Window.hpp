#pragma once
#include <SDL.h>

#include <string>

namespace Engine {

class Window {
public:
  Window(const std::string &title, int width, int height);
  ~Window();

  // No copying (unique resource)
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  // Moving is allowed
  Window(Window &&other) noexcept;
  Window &operator=(Window &&other) noexcept;

  [[nodiscard]] SDL_Window *getNativeHandle() const { return window; }

private:
  SDL_Window *window;
};

} 
