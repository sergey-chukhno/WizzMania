#pragma once
#include <SDL_ttf.h>
#include <string>

namespace Engine {

class Font {
public:
  Font(const std::string &filePath, int ptSize);
  ~Font();

  // No copy
  Font(const Font &) = delete;
  Font &operator=(const Font &) = delete;

  // Move allowed
  Font(Font &&other) noexcept;
  Font &operator=(Font &&other) noexcept;

  [[nodiscard]] TTF_Font *getNativeHandle() const { return font; }

private:
  TTF_Font *font;
};

} // namespace Engine
