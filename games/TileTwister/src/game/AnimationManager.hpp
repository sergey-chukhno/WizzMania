#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace Game {

struct Color {
  uint8_t r, g, b, a;
};

// Forward declaration for Color, assuming it's defined elsewhere or will be.
// If Color is a simple type like an alias for int or a struct defined in this
// file, this forward declaration might not be strictly necessary or might need
// adjustment. For now, assuming it's a struct/class.
struct Color;

struct Animation {
  enum class Type { Slide, Spawn, Merge, Shake, Score };

  Type type;

  // Timer
  float timer = 0.0f;
  float duration = 0.0f;
  bool finished = false;

  // Slide: From/To Coordinates (Global Pixel Coords)
  float startX = 0, startY = 0;
  float endX = 0, endY = 0;

  // Scale for Spawn/Merge
  float startScale = 1.0f;
  float endScale = 1.0f;
  int value = 0;

  // Shake Properties
  float shakeOffsetX = 0.0f;

  // Score Properties
  std::string text;
  Color color = {255, 255, 255, 255};

  // Helpers
  float getProgress() const { return std::min(timer / duration, 1.0f); }
};

class AnimationManager {
public:
  AnimationManager() = default;

  void addAnimation(const Animation &anim);
  void update(float dt);
  void clear();

  // Access active animations for rendering
  const std::vector<Animation> &getAnimations() const;
  bool isAnimating() const;
  bool hasBlockingAnimations() const;

private:
  std::vector<Animation> m_animations;

  // Easing Functions
  static float easeOutCubic(float t) { return 1.0f - std::pow(1.0f - t, 3.0f); }

  static float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
  }
};

} // namespace Game
