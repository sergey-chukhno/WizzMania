#include "Texture.hpp"
#include "Renderer.hpp"
#include <SDL_image.h>
#include <stdexcept>

namespace Engine {

Texture::Texture(Renderer &renderer, const std::string &path)
    : m_texture(nullptr), m_width(0), m_height(0) {

  SDL_Surface *surface = IMG_Load(path.c_str());
  if (!surface) {
    throw std::runtime_error("IMG_Load failed: " + std::string(IMG_GetError()));
  }

  m_texture = SDL_CreateTextureFromSurface(renderer.getInternal(), surface);
  if (!m_texture) {
    SDL_FreeSurface(surface);
    throw std::runtime_error("CreateTexture failed: " +
                             std::string(SDL_GetError()));
  }

  m_width = surface->w;
  m_height = surface->h;
  SDL_FreeSurface(surface);
}

Texture::Texture(Renderer &renderer, const std::string &path, uint8_t r,
                 uint8_t g, uint8_t b, int threshold)
    : m_texture(nullptr), m_width(0), m_height(0) {

  SDL_Surface *surface = IMG_Load(path.c_str());
  if (!surface) {
    throw std::runtime_error("Failed to load texture: " + path +
                             " IMG_GetError: " + IMG_GetError());
  }

  // Fuzzy Keying Logic
  if (threshold > 0) {
    // Must ensure surface is editable (RGBA)
    SDL_Surface *formattedSurf =
        SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(surface);
    surface = formattedSurf;

    // Lock Surface
    SDL_LockSurface(surface);
    Uint32 *pixels = (Uint32 *)surface->pixels;
    int pixelCount = (surface->pitch / 4) * surface->h;

    // Target Color
    // Note: We need to map RGB to the format
    Uint32 targetKey = SDL_MapRGBA(surface->format, r, g, b, 255);
    uint8_t tr, tg, tb, ta;
    SDL_GetRGBA(targetKey, surface->format, &tr, &tg, &tb, &ta);

    for (int i = 0; i < pixelCount; ++i) {
      uint8_t pr, pg, pb, pa;
      SDL_GetRGBA(pixels[i], surface->format, &pr, &pg, &pb, &pa);

      int dr = abs(pr - tr);
      int dg = abs(pg - tg);
      int db = abs(pb - tb);

      if (dr <= threshold && dg <= threshold && db <= threshold) {
        // Make Transparent
        pixels[i] = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
      }
    }
    SDL_UnlockSurface(surface);
  } else {
    // Exact Match (Standard SDL)
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, r, g, b));
  }

  m_texture = SDL_CreateTextureFromSurface(renderer.getInternal(), surface);
  if (!m_texture) {
    SDL_FreeSurface(surface);
    throw std::runtime_error("Failed to create texture from surface: " + path);
  }

  m_width = surface->w;
  m_height = surface->h;

  SDL_FreeSurface(surface);
}

Texture::~Texture() {
  if (m_texture) {
    SDL_DestroyTexture(m_texture);
  }
}

Texture::Texture(Texture &&other) noexcept
    : m_texture(other.m_texture), m_width(other.m_width),
      m_height(other.m_height) {
  other.m_texture = nullptr;
}

Texture &Texture::operator=(Texture &&other) noexcept {
  if (this != &other) {
    if (m_texture)
      SDL_DestroyTexture(m_texture);
    m_texture = other.m_texture;
    m_width = other.m_width;
    m_height = other.m_height;
    other.m_texture = nullptr;
  }
  return *this;
}

void Texture::setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (m_texture)
    SDL_SetTextureColorMod(m_texture, r, g, b);
}

void Texture::setAlpha(uint8_t a) { SDL_SetTextureAlphaMod(m_texture, a); }

void Texture::setBlendMode(SDL_BlendMode blending) {
  SDL_SetTextureBlendMode(m_texture, blending);
}

} // namespace Engine
