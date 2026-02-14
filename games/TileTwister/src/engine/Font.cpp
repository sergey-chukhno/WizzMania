#include "Font.hpp"
#include <stdexcept>

namespace Engine {

Font::Font(const std::string &filePath, int ptSize) : font(nullptr) {
  font = TTF_OpenFont(filePath.c_str(), ptSize);
  if (!font) {
    throw std::runtime_error("Failed to load font: " + filePath +
                             ". Error: " + std::string(TTF_GetError()));
  }
}

Font::~Font() {
  if (font) {
    TTF_CloseFont(font);
  }
}

Font::Font(Font &&other) noexcept : font(other.font) { other.font = nullptr; }

Font &Font::operator=(Font &&other) noexcept {
  if (this != &other) {
    if (font)
      TTF_CloseFont(font);
    font = other.font;
    other.font = nullptr;
  }
  return *this;
}

} // namespace Engine
