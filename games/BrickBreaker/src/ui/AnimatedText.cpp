#include "ui/AnimatedText.h"
#include <algorithm>
#include <cmath>
#include <vector>

AnimatedText::AnimatedText(const sf::Font &font, const std::string &text,
                           unsigned int characterSize)
    : text_(font, text, characterSize), shadowText_(font, text, characterSize),
      currentAlpha_(TEXT_PULSE_MAX_ALPHA), pulseSpeed_(TEXT_PULSE_SPEED),
      minAlpha_(TEXT_PULSE_MIN_ALPHA), maxAlpha_(TEXT_PULSE_MAX_ALPHA),
      pulseDirection_(-1.0f) // Start decreasing
      ,
      glowEnabled_(true), shadowEnabled_(true),
      shadowOffset_(SHADOW_OFFSET_X, SHADOW_OFFSET_Y),
      baseColor_(255, 255, 255) {
  // Initialize shadow text
  shadowText_.setFillColor(sf::Color(0, 0, 0, 128)); // Dark with transparency
  shadowText_.setStyle(sf::Text::Bold);
}

void AnimatedText::update(float deltaTime) {
  if (glowEnabled_) {
    updatePulse(deltaTime);
  }
}

void AnimatedText::render(sf::RenderWindow &window) const {
  // Create mutable copies for rendering
  sf::Text shadowText = shadowText_;
  sf::Text mainText = text_;

  // Render shadow first (if enabled)
  if (shadowEnabled_) {
    shadowText.setPosition(text_.getPosition() + shadowOffset_);
    window.draw(shadowText);
  }

  // Render glow layers (if enabled)
  if (glowEnabled_) {
    renderGlow(window);
  }

  // Render main text with current alpha
  sf::Color textColor = baseColor_;
  textColor.a = static_cast<unsigned char>(currentAlpha_ * 255.0f);
  mainText.setFillColor(textColor);
  window.draw(mainText);
}

void AnimatedText::setPosition(const sf::Vector2f &position) {
  text_.setPosition(position);
  shadowText_.setPosition(position + shadowOffset_);
}

void AnimatedText::setString(const std::string &text) {
  text_.setString(text);
  shadowText_.setString(text);
}

void AnimatedText::setFillColor(const sf::Color &color) {
  baseColor_ = color;
  text_.setFillColor(color);
}

void AnimatedText::setCharacterSize(unsigned int size) {
  text_.setCharacterSize(size);
  shadowText_.setCharacterSize(size);
}

void AnimatedText::setPulseSpeed(float speed) { pulseSpeed_ = speed; }

void AnimatedText::setPulseRange(float minAlpha, float maxAlpha) {
  minAlpha_ = clamp(minAlpha, 0.0f, 1.0f);
  maxAlpha_ = clamp(maxAlpha, 0.0f, 1.0f);
  currentAlpha_ = clamp(currentAlpha_, minAlpha_, maxAlpha_);
}

void AnimatedText::setGlowEnabled(bool enabled) { glowEnabled_ = enabled; }

void AnimatedText::setShadowEnabled(bool enabled) { shadowEnabled_ = enabled; }

sf::FloatRect AnimatedText::getLocalBounds() const {
  return text_.getLocalBounds();
}

void AnimatedText::setOrigin(const sf::Vector2f &origin) {
  text_.setOrigin(origin);
  shadowText_.setOrigin(origin);
}

void AnimatedText::updatePulse(float deltaTime) {
  // Update alpha based on pulse direction
  currentAlpha_ += pulseSpeed_ * pulseDirection_ * deltaTime;

  // Reverse direction if alpha reaches min or max
  if (currentAlpha_ >= maxAlpha_) {
    currentAlpha_ = maxAlpha_;
    pulseDirection_ = -1.0f;
  } else if (currentAlpha_ <= minAlpha_) {
    currentAlpha_ = minAlpha_;
    pulseDirection_ = 1.0f;
  }

  // Clamp alpha to be safe
  currentAlpha_ = clamp(currentAlpha_, minAlpha_, maxAlpha_);
}

void AnimatedText::renderGlow(sf::RenderWindow &window) const {
  // Render multiple glow layers for prominent cyberpunk bloom effect
  // Render in 8 directions for full glow effect
  std::vector<sf::Vector2f> directions = {
      sf::Vector2f(1.0f, 0.0f),  // Right
      sf::Vector2f(-1.0f, 0.0f), // Left
      sf::Vector2f(0.0f, 1.0f),  // Down
      sf::Vector2f(0.0f, -1.0f), // Up
      sf::Vector2f(0.7f, 0.7f),  // Down-right
      sf::Vector2f(-0.7f, 0.7f), // Down-left
      sf::Vector2f(0.7f, -0.7f), // Up-right
      sf::Vector2f(-0.7f, -0.7f) // Up-left
  };

  for (int layer = GLOW_LAYER_COUNT; layer >= 1; --layer) {
    float offset =
        static_cast<float>(layer) * 2.0f; // Larger offsets for more bloom
    float layerAlpha = currentAlpha_ * (0.5f - layer * 0.12f);
    layerAlpha = std::max(0.0f, layerAlpha);

    // Brighten the glow color for more vibrant effect
    sf::Color glowColor = baseColor_;
    glowColor.r = static_cast<unsigned char>(
        std::min(255, static_cast<int>(glowColor.r * 1.2f)));
    glowColor.g = static_cast<unsigned char>(
        std::min(255, static_cast<int>(glowColor.g * 1.2f)));
    glowColor.b = static_cast<unsigned char>(
        std::min(255, static_cast<int>(glowColor.b * 1.2f)));
    glowColor.a = static_cast<unsigned char>(layerAlpha * 255.0f);

    for (const auto &dir : directions) {
      sf::Text glowText = text_;
      glowText.setFillColor(glowColor);
      glowText.setPosition(text_.getPosition() +
                           sf::Vector2f(dir.x * offset, dir.y * offset));
      window.draw(glowText);
    }
  }
}

float AnimatedText::clamp(float value, float min, float max) {
  return std::max(min, std::min(max, value));
}
