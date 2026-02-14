#pragma once
#include "Font.hpp"
#include "Texture.hpp" // Added
#include "Tile.hpp"
#include "Window.hpp"
#include <SDL.h>

namespace Engine {

struct Color {
  uint8_t r, g, b, a;
};

class Renderer {
public:
  explicit Renderer(const Window &window, int logicalWidth, int logicalHeight);
  ~Renderer();

  // No copy
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Move allowed
  Renderer(Renderer &&other) noexcept;
  Renderer &operator=(Renderer &&other) noexcept;

  [[nodiscard]] SDL_Renderer *getInternal() const { return renderer; }

  // Drawing Primitives
  void clear();
  void present();
  void setDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
  void drawFillRect(int x, int y, int w, int h);
  void drawTexture(const Texture &texture, const SDL_Rect &dstRect);
  void drawTexture(const Texture &texture, const SDL_Rect &srcRect,
                   const SDL_Rect &dstRect); // Added overload
  // Text Rendering
  void drawText(const std::string &text, const Font &font, int x, int y,
                uint8_t r, uint8_t g, uint8_t b, uint8_t a);

  // Helper to center text in a rect
  void drawTextCentered(const std::string &text, const Font &font, int cx,
                        int cy, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

private:
  SDL_Renderer *renderer;
};

} // namespace Engine
