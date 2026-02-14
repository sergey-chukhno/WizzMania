#include "ui/Button.h"
#include <algorithm>
#include <cmath>

Button::Button(const sf::Font &font, const std::string &text,
               const sf::Vector2f &position, const sf::Vector2f &size)
    : position_(position), size_(size), baseScale_(1.0f, 1.0f),
      hoverScale_(1.0f), text_(font, text, 20),
      useAngledStyle_(true) // Default to angled cyberpunk style
      ,
      fillColor_(0, 30, 40, 180) // Subtle cyan-tinted dark background
      ,
      outlineColor_(0, 217, 255) // Cyan
      ,
      textColor_(0, 217, 255) // Cyan text to match outline
      ,
      hoverFillColor_(0, 60, 80, 200) // Brighter cyan for hover
      ,
      glowIntensity_(GLOW_INTENSITY_MIN), isHovered_(false), wasClicked_(false),
      clickFlashTime_(0.0f) {
  // Initialize button rectangle (fallback for non-angled)
  buttonRect_.setSize(size_);
  buttonRect_.setFillColor(fillColor_);
  buttonRect_.setOutlineColor(outlineColor_);
  buttonRect_.setOutlineThickness(OUTLINE_THICKNESS);
  buttonRect_.setOrigin(sf::Vector2f(size_.x / 2.0f, size_.y / 2.0f));
  buttonRect_.setPosition(position_);

  // Initialize angled shape
  updateAngledShape();

  // Initialize text
  text_.setFillColor(textColor_);
  text_.setStyle(sf::Text::Bold);

  // Center text on button
  sf::FloatRect textBounds = text_.getLocalBounds();
  text_.setOrigin(
      sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
  text_.setPosition(position_);
}

void Button::update(float deltaTime) {
  // Update hover scale and glow intensity
  updateHover(deltaTime);

  // Update click flash
  if (clickFlashTime_ > 0.0f) {
    clickFlashTime_ -= deltaTime;
    if (clickFlashTime_ < 0.0f) {
      clickFlashTime_ = 0.0f;
      wasClicked_ = false;
    }
  }

  // Update fill color based on hover state
  if (useAngledStyle_) {
    sf::Color currentFill = isHovered_ ? hoverFillColor_ : fillColor_;
    angledShape_.setFillColor(currentFill);
  }

  // Update button rectangle scale
  float currentScale = hoverScale_;
  buttonRect_.setScale(sf::Vector2f(currentScale, currentScale));
  angledShape_.setScale(sf::Vector2f(currentScale, currentScale));
}

void Button::render(sf::RenderWindow &window) const {
  if (useAngledStyle_) {
    // Render subtle glow for angled style
    renderAngledGlow(window);

    // Render angled button
    window.draw(angledShape_);
  } else {
    // Render standard rectangle button
    window.draw(buttonRect_);
  }

  // Render text
  window.draw(text_);

  // Render click flash (white overlay) - fades out over time
  if (clickFlashTime_ > 0.0f) {
    float flashAlpha = (clickFlashTime_ / CLICK_FLASH_DURATION) * 150.0f;
    flashAlpha = std::max(0.0f, std::min(255.0f, flashAlpha));

    if (useAngledStyle_) {
      sf::ConvexShape flashShape = createAngledShape(0);
      flashShape.setFillColor(
          sf::Color(0, 217, 255, static_cast<unsigned char>(flashAlpha)));
      flashShape.setOutlineThickness(0);
      flashShape.setScale(sf::Vector2f(hoverScale_, hoverScale_));
      window.draw(flashShape);
    } else {
      sf::RectangleShape flashRect(size_);
      flashRect.setFillColor(
          sf::Color(255, 255, 255, static_cast<unsigned char>(flashAlpha)));
      flashRect.setOrigin(sf::Vector2f(size_.x / 2.0f, size_.y / 2.0f));
      flashRect.setPosition(position_);
      flashRect.setScale(sf::Vector2f(hoverScale_, hoverScale_));
      window.draw(flashRect);
    }
  }
}

void Button::handleEvent(const sf::Event &event,
                         const sf::RenderWindow &window) {
  // Get mouse position in window coordinates
  sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
  sf::Vector2f mousePos = window.mapPixelToCoords(mousePixelPos);

  // Check if mouse is over button
  isHovered_ = getGlobalBounds().contains(mousePos);

  // Handle mouse button press
  if (auto *mouseButton = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mouseButton->button == sf::Mouse::Button::Left && isHovered_) {
      wasClicked_ = true;
      clickFlashTime_ = CLICK_FLASH_DURATION;

      // Trigger callback
      if (onClickCallback_) {
        onClickCallback_();
      }
    }
  }
}

void Button::setOnClick(std::function<void()> callback) {
  onClickCallback_ = callback;
}

void Button::setPosition(const sf::Vector2f &position) {
  position_ = position;
  buttonRect_.setPosition(position_);
  angledShape_.setPosition(position_);
  text_.setPosition(position_);
}

void Button::setSize(const sf::Vector2f &size) {
  size_ = size;
  buttonRect_.setSize(size_);
  buttonRect_.setOrigin(sf::Vector2f(size_.x / 2.0f, size_.y / 2.0f));
  updateAngledShape();
}

void Button::setText(const std::string &text) {
  text_.setString(text);

  // Re-center text
  sf::FloatRect textBounds = text_.getLocalBounds();
  text_.setOrigin(
      sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
  text_.setPosition(position_);
}

void Button::setColors(const sf::Color &fillColor,
                       const sf::Color &outlineColor,
                       const sf::Color &textColor) {
  fillColor_ = fillColor;
  outlineColor_ = outlineColor;
  textColor_ = textColor;
  hoverFillColor_ =
      sf::Color(outlineColor.r, outlineColor.g, outlineColor.b, 40);

  buttonRect_.setFillColor(fillColor_);
  buttonRect_.setOutlineColor(outlineColor_);
  angledShape_.setFillColor(fillColor_);
  angledShape_.setOutlineColor(outlineColor_);
  text_.setFillColor(textColor_);
}

void Button::setAngledStyle(bool enabled) { useAngledStyle_ = enabled; }

sf::FloatRect Button::getGlobalBounds() const {
  if (useAngledStyle_) {
    return angledShape_.getGlobalBounds();
  }
  return buttonRect_.getGlobalBounds();
}

bool Button::isHovered() const { return isHovered_; }

void Button::updateAngledShape() {
  angledShape_ = createAngledShape(0);
  angledShape_.setFillColor(fillColor_);
  angledShape_.setOutlineColor(outlineColor_);
  angledShape_.setOutlineThickness(OUTLINE_THICKNESS);
  angledShape_.setPosition(position_);
}

sf::ConvexShape Button::createAngledShape(float sizeOffset) const {
  // Create a hexagonal shape with clipped corners (cyberpunk style)
  // 8 points for clipped corners on all 4 corners
  sf::ConvexShape shape(8);

  float w = size_.x / 2.0f + sizeOffset;
  float h = size_.y / 2.0f + sizeOffset;
  float cut = CORNER_CUT_SIZE;

  // Define points clockwise from top-left, with cut corners
  // Top-left corner (cut)
  shape.setPoint(0, sf::Vector2f(-w + cut, -h));
  // Top-right corner (cut)
  shape.setPoint(1, sf::Vector2f(w - cut, -h));
  shape.setPoint(2, sf::Vector2f(w, -h + cut));
  // Bottom-right corner (cut)
  shape.setPoint(3, sf::Vector2f(w, h - cut));
  shape.setPoint(4, sf::Vector2f(w - cut, h));
  // Bottom-left corner (cut)
  shape.setPoint(5, sf::Vector2f(-w + cut, h));
  shape.setPoint(6, sf::Vector2f(-w, h - cut));
  // Back to top-left
  shape.setPoint(7, sf::Vector2f(-w, -h + cut));

  return shape;
}

void Button::updateHover(float deltaTime) {
  // Target values based on hover state
  float targetScale = isHovered_ ? HOVER_SCALE : 1.0f;
  float targetGlow = isHovered_ ? GLOW_INTENSITY_MAX : GLOW_INTENSITY_MIN;

  // Lerp towards target values
  float lerpFactor = HOVER_LERP_SPEED * deltaTime;
  hoverScale_ = lerp(hoverScale_, targetScale, lerpFactor);
  glowIntensity_ = lerp(glowIntensity_, targetGlow, lerpFactor);

  // Clamp values
  hoverScale_ = clamp(hoverScale_, 1.0f, HOVER_SCALE);
  glowIntensity_ =
      clamp(glowIntensity_, GLOW_INTENSITY_MIN, GLOW_INTENSITY_MAX);
}

void Button::renderAngledGlow(sf::RenderWindow &window) const {
  // Render subtle glow layers for cyberpunk effect
  for (int i = 2; i >= 0; --i) {
    float offset = (i + 1) * 3.0f;
    float alpha = glowIntensity_ * (30.0f - i * 8.0f);
    alpha = clamp(alpha, 0.0f, 255.0f);

    sf::ConvexShape glowShape = createAngledShape(offset);
    glowShape.setFillColor(sf::Color(outlineColor_.r, outlineColor_.g,
                                     outlineColor_.b,
                                     static_cast<unsigned char>(alpha)));
    glowShape.setOutlineThickness(0);
    glowShape.setPosition(position_);
    glowShape.setScale(sf::Vector2f(hoverScale_, hoverScale_));

    window.draw(glowShape);
  }
}

float Button::lerp(float a, float b, float t) {
  return a + (b - a) * std::min(1.0f, std::max(0.0f, t));
}

float Button::clamp(float value, float min, float max) {
  return std::max(min, std::min(max, value));
}
