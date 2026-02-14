#include "Renderer.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace Engine {

Renderer::Renderer(const Window &window, int logicalWidth, int logicalHeight)
    : renderer(nullptr) {
  renderer =
      SDL_CreateRenderer(window.getNativeHandle(), -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    throw std::runtime_error("Renderer could not be created! SDL_Error: " +
                             std::string(SDL_GetError()));
  }

  if (SDL_RenderSetLogicalSize(renderer, logicalWidth, logicalHeight) < 0) {
    // Log warning but don't fail, usually fine.
    std::cerr << "Warning: SDL_RenderSetLogicalSize failed: " << SDL_GetError()
              << std::endl;
  }
}

Renderer::~Renderer() {
  if (renderer) {
    SDL_DestroyRenderer(renderer);
  }
}

Renderer::Renderer(Renderer &&other) noexcept : renderer(other.renderer) {
  other.renderer = nullptr;
}

Renderer &Renderer::operator=(Renderer &&other) noexcept {
  if (this != &other) {
    if (renderer)
      SDL_DestroyRenderer(renderer);
    renderer = other.renderer;
    other.renderer = nullptr;
  }
  return *this;
}

void Renderer::clear() { SDL_RenderClear(renderer); }

void Renderer::present() { SDL_RenderPresent(renderer); }

void Renderer::setDrawColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void Renderer::drawFillRect(int x, int y, int w, int h) {
  SDL_Rect rect;
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  SDL_RenderFillRect(renderer, &rect);
}

void Renderer::drawText(const std::string &text, const Font &font, int x, int y,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  SDL_Color color = {r, g, b, a};
  SDL_Surface *surface =
      TTF_RenderText_Blended(font.getNativeHandle(), text.c_str(), color);
  if (!surface) {
    // Handle error (log it)
    std::cerr << "TTF Render Error: " << TTF_GetError() << std::endl;
    return;
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    std::cerr << "Texture Create Error: " << SDL_GetError() << std::endl;
    SDL_FreeSurface(surface);
    return;
  }

  SDL_Rect dest;
  dest.x = x;
  dest.y = y;
  dest.w = surface->w;
  dest.h = surface->h;

  SDL_RenderCopy(renderer, texture, nullptr, &dest);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void Renderer::drawTextCentered(const std::string &text, const Font &font,
                                int cx, int cy, uint8_t r, uint8_t g, uint8_t b,
                                uint8_t a) {
  SDL_Color color = {r, g, b, a};

  // Render to surface to get dimensions
  SDL_Surface *surface =
      TTF_RenderText_Blended(font.getNativeHandle(), text.c_str(), color);
  if (!surface) {
    return;
  }

  int w = surface->w;
  int h = surface->h;

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    SDL_FreeSurface(surface);
    return;
  }

  // Centering Logic
  SDL_Rect dest;
  dest.x = cx - (w / 2);
  dest.y = cy - (h / 2);
  dest.w = w;
  dest.h = h;

  SDL_RenderCopy(renderer, texture, nullptr, &dest);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void Renderer::drawTexture(const Texture &texture, const SDL_Rect &dstRect) {
  SDL_RenderCopy(renderer, texture.get(), NULL, &dstRect);
}

void Renderer::drawTexture(const Texture &texture, const SDL_Rect &srcRect,
                           const SDL_Rect &dstRect) {
  SDL_RenderCopy(renderer, texture.get(), &srcRect, &dstRect);
}

} // namespace Engine
