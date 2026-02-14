#include "entities/Brick.h"
#include "core/FontManager.h"
#include "core/Game.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Brick::Brick(const sf::Vector2f &position, const sf::Vector2f &size,
             int maxHealth, const sf::Color &baseColor, float distanceFactor)
    : position_(position), size_(size), health_(maxHealth),
      maxHealth_(maxHealth), isDestroyed_(false),
      distanceFactor_(distanceFactor) // Store distance factor for gradient (0.0
                                      // = edge, 1.0 = center)
      ,
      shape_(size), baseColor_(baseColor), currentColor_(baseColor),
      healthText_(FontManager::getBodyFont(), "", HEALTH_TEXT_SIZE),
      rotationAngle_(
          static_cast<float>(std::rand() % 360)) // Random starting rotation
      ,
      rotationSpeed_(ROTATION_SPEED_MIN +
                     static_cast<float>(std::rand() %
                                        static_cast<int>(ROTATION_SPEED_MAX -
                                                         ROTATION_SPEED_MIN))),
      pulseAlpha_(PULSE_ALPHA_MAX), glowIntensity_(GLOW_INTENSITY_MAX),
      animationTime_(static_cast<float>(std::rand() % 100) /
                     100.0f) // Random starting animation time
{
  // Randomize rotation direction (clockwise or counter-clockwise)
  if (std::rand() % 2 == 0) {
    rotationSpeed_ = -rotationSpeed_;
  }

  // Initialize shape
  shape_.setFillColor(currentColor_);
  shape_.setOutlineColor(currentColor_);
  shape_.setOutlineThickness(OUTLINE_THICKNESS);

  // Set origin to center
  shape_.setOrigin(sf::Vector2f(size_.x / 2.0f, size_.y / 2.0f));
  shape_.setPosition(position_);
  shape_.setRotation(sf::degrees(rotationAngle_));

  // Initialize health text
  healthText_.setFillColor(HEALTH_TEXT_COLOR);
  healthText_.setStyle(sf::Text::Bold);
  updateHealthText();
  updateColor();
}

void Brick::update(float deltaTime) {
  if (isDestroyed_) {
    return;
  }

  // Update animations (rotation, pulse, glow)
  updateAnimations(deltaTime);

  // Update shape position and rotation
  shape_.setPosition(position_);
  shape_.setRotation(sf::degrees(rotationAngle_));

  // Update health text position
  healthText_.setPosition(position_);
}

void Brick::render(sf::RenderWindow &window) const {
  if (isDestroyed_) {
    return;
  }

  // Render enhanced glow effects first (behind main shape, with animation)
  // Glow uses current color with enhanced visibility
  renderGlow(window, shape_, currentColor_);

  // Render main shape (with rotation) - this will be on top of glow
  window.draw(shape_);

  // Render health indicator (ALWAYS visible)
  window.draw(healthText_);
}

bool Brick::takeDamage(int amount) {
  if (isDestroyed_) {
    return false;
  }

  health_ -= amount;

  if (health_ <= 0) {
    health_ = 0;
    isDestroyed_ = true;
    return true; // Brick destroyed
  }

  // Update color based on new health
  updateColor();

  // Update health text
  updateHealthText();

  return false; // Brick not destroyed
}

bool Brick::isDestroyed() const { return isDestroyed_; }

sf::FloatRect Brick::getBounds() const {
  if (isDestroyed_) {
    // SFML 3.0: FloatRect constructor takes position (Vector2f) and size
    // (Vector2f)
    return sf::FloatRect(sf::Vector2f(0, 0),
                         sf::Vector2f(0, 0)); // Empty bounds
  }

  // Calculate bounds from position and size (center-based)
  sf::Vector2f topLeft(position_.x - size_.x / 2.0f,
                       position_.y - size_.y / 2.0f);
  return sf::FloatRect(topLeft, size_);
}

sf::Vector2f Brick::getPosition() const { return position_; }

int Brick::getHealth() const { return health_; }

int Brick::getMaxHealth() const { return maxHealth_; }

void Brick::setPosition(const sf::Vector2f &position) {
  position_ = position;
  shape_.setPosition(position_);
  healthText_.setPosition(position_);
}

sf::Color Brick::getBaseColor() const { return baseColor_; }

void Brick::updateColor() {
  // Use distance factor for gradient (based on position from block center)
  // distanceFactor = 1.0 (center) -> darker color
  // distanceFactor = 0.0 (edge) -> lighter color
  // This creates a gradient based on initial position, not current health
  currentColor_ = adjustBrightness(baseColor_, distanceFactor_);

  // Update shape color
  shape_.setFillColor(currentColor_);
  shape_.setOutlineColor(currentColor_);
}

void Brick::updateHealthText() {
  std::stringstream ss;
  ss << health_;
  healthText_.setString(ss.str());

  // Center text horizontally and vertically
  sf::FloatRect textBounds = healthText_.getLocalBounds();
  healthText_.setOrigin(
      sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
  healthText_.setPosition(position_);
}

void Brick::renderGlow(sf::RenderWindow &window,
                       const sf::RectangleShape &shape,
                       const sf::Color &baseColor) const {
  // Render multiple glow layers with decreasing opacity (enhanced for strong
  // asteroid effect)
  for (int i = 0; i < GLOW_LAYERS; ++i) {
    float scale = 1.0f + (static_cast<float>(i + 1) * GLOW_SCALE_STEP);

    // Calculate alpha with pulse modulation (much more visible glow)
    // Start with higher base alpha and decrease more gradually
    float baseAlpha =
        static_cast<float>(GLOW_ALPHA_BASE - (i * GLOW_ALPHA_DECREMENT));
    baseAlpha = std::max(0.0f, baseAlpha); // Ensure non-negative

    // Apply pulse and glow intensity modulation
    float alpha = baseAlpha * pulseAlpha_ * glowIntensity_;
    alpha = std::max(0.0f, std::min(255.0f, alpha));

    sf::RectangleShape glowShape = shape;
    sf::Vector2f originalSize = glowShape.getSize();
    sf::Vector2f scaledSize =
        sf::Vector2f(originalSize.x * scale, originalSize.y * scale);
    glowShape.setSize(scaledSize);

    // Set origin to center (matching main shape origin)
    glowShape.setOrigin(sf::Vector2f(scaledSize.x / 2.0f, scaledSize.y / 2.0f));

    // Apply same rotation and position as main shape (for rotating glow effect)
    glowShape.setRotation(shape.getRotation());
    glowShape.setPosition(shape.getPosition());

    // Apply glow intensity to color (brighten more for stronger asteroid
    // effect) Use a wider range for more visible color variation
    float colorIntensity =
        0.7f + (glowIntensity_ * 0.3f); // Range: 0.7-1.0 (brighter)
    unsigned char r = static_cast<unsigned char>(
        std::min(255.0f, baseColor.r * colorIntensity));
    unsigned char g = static_cast<unsigned char>(
        std::min(255.0f, baseColor.g * colorIntensity));
    unsigned char b = static_cast<unsigned char>(
        std::min(255.0f, baseColor.b * colorIntensity));

    // Add brightness boost to glow layers for more visibility (makes glow stand
    // out)
    r = static_cast<unsigned char>(std::min(255, static_cast<int>(r) + 15));
    g = static_cast<unsigned char>(std::min(255, static_cast<int>(g) + 15));
    b = static_cast<unsigned char>(std::min(255, static_cast<int>(b) + 15));

    glowShape.setFillColor(
        sf::Color(r, g, b, static_cast<unsigned char>(alpha)));
    glowShape.setOutlineColor(
        sf::Color(r, g, b, static_cast<unsigned char>(alpha)));
    glowShape.setOutlineThickness(OUTLINE_THICKNESS * scale *
                                  1.2f); // Thicker outline for glow

    window.draw(glowShape);
  }
}

sf::Color Brick::adjustBrightness(const sf::Color &color,
                                  float distanceFactor) {
  // Brightness adjustment: darker = center, lighter = edges (VERY pronounced
  // gradient) distanceFactor = 1.0 (center) -> darker distanceFactor = 0.0
  // (edge) -> lighter

  // VERY pronounced gradient for asteroid-like appearance
  // Use linear interpolation for more consistent gradient
  float minBrightness = 0.15f; // Much darker for center (15% brightness)
  float maxBrightness = 1.0f;  // Full brightness for edges (100% brightness)

  // Linear interpolation: center = darker, edge = lighter
  float brightness =
      minBrightness + (1.0f - distanceFactor) * (maxBrightness - minBrightness);

  // Apply brightness to RGB components (clamp to [0, 255])
  float rFloat = color.r * brightness;
  float gFloat = color.g * brightness;
  float bFloat = color.b * brightness;

  unsigned char r =
      static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, rFloat)));
  unsigned char g =
      static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, gFloat)));
  unsigned char b =
      static_cast<unsigned char>(std::max(0.0f, std::min(255.0f, bFloat)));

  return sf::Color(r, g, b, color.a);
}

void Brick::updateAnimations(float deltaTime) {
  // Update animation time
  animationTime_ += deltaTime;

  // Update rotation (asteroid-like spinning)
  rotationAngle_ += rotationSpeed_ * deltaTime;

  // Wrap rotation to [0, 360) degrees
  while (rotationAngle_ >= 360.0f) {
    rotationAngle_ -= 360.0f;
  }
  while (rotationAngle_ < 0.0f) {
    rotationAngle_ += 360.0f;
  }

  // Update pulse animation using sine wave for smooth glow (asteroid-like
  // pulsing) Use sine wave: sin(2π * frequency * time) maps to [-1, 1] Map to
  // [PULSE_ALPHA_MIN, PULSE_ALPHA_MAX]
  float sineValue =
      std::sin(2.0f * static_cast<float>(M_PI) * PULSE_SPEED * animationTime_);
  pulseAlpha_ = PULSE_ALPHA_MIN +
                (sineValue + 1.0f) / 2.0f * (PULSE_ALPHA_MAX - PULSE_ALPHA_MIN);

  // Update glow intensity (synchronized with pulse)
  glowIntensity_ =
      PULSE_ALPHA_MIN + (pulseAlpha_ - PULSE_ALPHA_MIN) *
                            ((GLOW_INTENSITY_MAX - GLOW_INTENSITY_MIN) /
                             (PULSE_ALPHA_MAX - PULSE_ALPHA_MIN));
}
