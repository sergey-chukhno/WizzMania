#include "Window.hpp"
#include <stdexcept>

namespace Engine {

Window::Window(const std::string &title, int width, int height)
    : window(nullptr) {
  window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, width, height,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!window) {
    throw std::runtime_error("Window could not be created! SDL_Error: " +
                             std::string(SDL_GetError()));
  }
}

Window::~Window() {
  if (window) {
    SDL_DestroyWindow(window);
  }
}

Window::Window(Window &&other) noexcept : window(other.window) {
  other.window = nullptr;
}

Window &Window::operator=(Window &&other) noexcept {
  if (this != &other) {
    if (window)
      SDL_DestroyWindow(window);
    window = other.window;
    other.window = nullptr;
  }
  return *this;
}

} 
