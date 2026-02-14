#include "entities/Cannon.h"
#include "core/AudioManager.h"
#include "core/FontManager.h"
#include "core/Game.h"
#include <cmath>
#include <exception>
#include <iostream>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cannon::Cannon(const sf::Vector2f &position, unsigned int projectileCount)
    : position_(position), angle_(DEFAULT_ANGLE), angleDirection_(0.0f),
      useMouseControl_(true) // Mouse control enabled by default
      ,
      corePulseAlpha_(CORE_PULSE_MAX_ALPHA),
      corePulseDirection_(-1.0f) // Start decreasing
      ,
      glowIntensity_(GLOW_INTENSITY_MAX), shootingEffectTimer_(0.0f),
      isShooting_(false), barrelRedPulseAlpha_(0.0f), lightningGlowAlpha_(0.0f),
      pulseTime_(0.0f), flameAnimationTime_(0.0f),
      baseBody_(sf::Vector2f(BASE_WIDTH, BASE_HEIGHT)),
      baseLeftPanel_(sf::Vector2f(BASE_PANEL_WIDTH, BASE_PANEL_HEIGHT)),
      baseRightPanel_(sf::Vector2f(BASE_PANEL_WIDTH, BASE_PANEL_HEIGHT)),
      baseCore_(BASE_CORE_RADIUS),
      baseFrontPanel_(sf::Vector2f(BASE_FRONT_WIDTH, BASE_FRONT_HEIGHT)),
      barrelMain_(sf::Vector2f(BARREL_LENGTH, BARREL_WIDTH)),
      barrelMuzzle_(sf::Vector2f(BARREL_MUZZLE_LENGTH, BARREL_MUZZLE_WIDTH)),
      barrelRing1_(BARREL_RING_RADIUS), barrelRing2_(BARREL_RING_RADIUS),
      barrelFin1_(sf::Vector2f(BARREL_FIN_WIDTH, BARREL_FIN_LENGTH)),
      barrelFin2_(sf::Vector2f(BARREL_FIN_WIDTH, BARREL_FIN_LENGTH)),
      counterText_(FontManager::getBodyFont(), "", 20),
      projectileCount_(projectileCount) {
  try {
    // Initialize base components
    initializeBase();

    // Initialize barrel components
    initializeBarrel();

    // Initialize counter text
    counterText_.setFillColor(COUNTER_TEXT_COLOR);
    counterText_.setStyle(sf::Text::Bold);
    updateCounterText();
    counterText_.setPosition(position_ + sf::Vector2f(0, COUNTER_OFFSET_Y));
  } catch (const std::exception &e) {
    std::cerr << "Error in Cannon constructor: " << e.what() << std::endl;
    throw;
  }
}

void Cannon::update(float deltaTime, const sf::RenderWindow &window) {
  // Update angle from mouse if mouse control is enabled
  if (useMouseControl_) {
    updateAngleFromMouse(window);
  } else {
    // Update angle based on keyboard input
    float angleChange = angleDirection_ * ANGLE_SPEED * deltaTime;
    angle_ += angleChange;
  }

  // Clamp angle to bounds (-45° to +45°)
  clampAngle();

  // Update animations (pulse, glow)
  updateAnimations(deltaTime);

  // Update shooting effects
  updateShootingEffects(deltaTime);

  // Update barrel rotation and position
  updateBarrelTransform();

  // Update counter text position (below base)
  counterText_.setPosition(position_ + sf::Vector2f(0, COUNTER_OFFSET_Y));
}

void Cannon::render(sf::RenderWindow &window) const {
  // Render white lightning glow on boundaries when shooting (behind everything)
  if (lightningGlowAlpha_ > 0.0f) {
    renderLightningGlow(window);
  }

  // Render glow effects for base components (behind main shapes)
  renderGlow(window, baseBody_, Game::NEON_CYAN, glowIntensity_);
  renderGlow(window, baseLeftPanel_, Game::NEON_PURPLE, glowIntensity_ * 0.8f);
  renderGlow(window, baseRightPanel_, Game::NEON_PURPLE, glowIntensity_ * 0.8f);
  renderGlow(window, baseFrontPanel_, Game::NEON_PINK, glowIntensity_ * 0.9f);

  // Render glow for barrel components (behind main shapes)
  renderGlow(window, barrelMain_, Game::NEON_CYAN, glowIntensity_);
  renderGlow(window, barrelMuzzle_, Game::NEON_PURPLE, glowIntensity_ * 0.9f);

  // Render base components (stationary, in order from back to front)
  window.draw(baseBody_);
  window.draw(baseLeftPanel_);
  window.draw(baseRightPanel_);
  window.draw(baseFrontPanel_);

  // Render energy core glow (behind core)
  // Create a temporary core shape for glow rendering with current pulse alpha
  sf::CircleShape coreForGlow = baseCore_;
  coreForGlow.setFillColor(
      sf::Color(Game::NEON_PINK.r, Game::NEON_PINK.g, Game::NEON_PINK.b,
                static_cast<unsigned char>(corePulseAlpha_ * 255.0f)));
  renderGlow(window, coreForGlow, Game::NEON_PINK, corePulseAlpha_);

  // Render energy core with pulsating alpha
  sf::CircleShape core = baseCore_;
  sf::Color coreColor = Game::NEON_PINK;
  coreColor.a = static_cast<unsigned char>(corePulseAlpha_ * 255.0f);
  core.setFillColor(coreColor);
  window.draw(core);

  // Render barrel components with red pulse effect when shooting
  // Barrel main with red pulse overlay - make red more dominant when shooting
  sf::RectangleShape barrelMainWithRed = barrelMain_;
  if (barrelRedPulseAlpha_ > 0.0f) {
    // Stronger red blend - interpolate between cyan and red based on pulse
    // alpha
    float redBlend = barrelRedPulseAlpha_; // 0.0 = cyan, 1.0 = red
    barrelMainWithRed.setFillColor(sf::Color(
        static_cast<unsigned char>(Game::NEON_CYAN.r * (1.0f - redBlend) +
                                   Game::NEON_PINK.r * redBlend),
        static_cast<unsigned char>(Game::NEON_CYAN.g * (1.0f - redBlend) +
                                   Game::NEON_PINK.g * redBlend),
        static_cast<unsigned char>(Game::NEON_CYAN.b * (1.0f - redBlend) +
                                   Game::NEON_PINK.b * redBlend)));
  }
  window.draw(barrelMainWithRed);

  // Barrel muzzle with red pulse overlay - make red more dominant when shooting
  sf::RectangleShape barrelMuzzleWithRed = barrelMuzzle_;
  if (barrelRedPulseAlpha_ > 0.0f) {
    // Stronger red blend - interpolate between cyan and red based on pulse
    // alpha
    float redBlend = barrelRedPulseAlpha_; // 0.0 = cyan, 1.0 = red
    barrelMuzzleWithRed.setFillColor(sf::Color(
        static_cast<unsigned char>(Game::NEON_PURPLE.r * (1.0f - redBlend) +
                                   Game::NEON_PINK.r * redBlend),
        static_cast<unsigned char>(Game::NEON_PURPLE.g * (1.0f - redBlend) +
                                   Game::NEON_PINK.g * redBlend),
        static_cast<unsigned char>(Game::NEON_PURPLE.b * (1.0f - redBlend) +
                                   Game::NEON_PINK.b * redBlend)));
  }

  window.draw(barrelRing1_);
  window.draw(barrelRing2_);
  window.draw(barrelFin1_);
  window.draw(barrelFin2_);
  window.draw(barrelMuzzleWithRed);

  // Render flame effect at muzzle when shooting (on top of barrel)
  if (isShooting_ && shootingEffectTimer_ > 0.0f) {
    renderFlameEffect(window);
  }

  // Render counter text (below base)
  window.draw(counterText_);
}

void Cannon::handleInput(const sf::Event &event,
                         const sf::RenderWindow & /* window */) {
  // Handle keyboard input
  if (auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    if (keyPressed->code == sf::Keyboard::Key::A ||
        keyPressed->code == sf::Keyboard::Key::Left) {
      angleDirection_ = -1.0f;  // Rotate left (counter-clockwise)
      useMouseControl_ = false; // Disable mouse control when keyboard is used
    } else if (keyPressed->code == sf::Keyboard::Key::D ||
               keyPressed->code == sf::Keyboard::Key::Right) {
      angleDirection_ = 1.0f;   // Rotate right (clockwise)
      useMouseControl_ = false; // Disable mouse control when keyboard is used
    }
  } else if (auto *keyReleased = event.getIf<sf::Event::KeyReleased>()) {
    if (keyReleased->code == sf::Keyboard::Key::A ||
        keyReleased->code == sf::Keyboard::Key::Left ||
        keyReleased->code == sf::Keyboard::Key::D ||
        keyReleased->code == sf::Keyboard::Key::Right) {
      angleDirection_ = 0.0f; // Stop rotation
      useMouseControl_ =
          true; // Re-enable mouse control when keyboard is released
    }
  }

  // Handle mouse movement (enable mouse control, actual update happens in
  // update())
  if (event.is<sf::Event::MouseMoved>()) {
    useMouseControl_ = true; // Enable mouse control when mouse moves
    angleDirection_ = 0.0f;  // Disable keyboard rotation
  }
}

sf::FloatRect Cannon::getBounds() const {
  // Return bounds of the base body (cannon is fixed, so base bounds are
  // sufficient)
  return baseBody_.getGlobalBounds();
}

sf::Vector2f Cannon::getPosition() const { return position_; }

float Cannon::getAngle() const { return angle_; }

sf::Vector2f Cannon::getShootDirection() const {
  return angleToDirection(angle_);
}

unsigned int Cannon::getProjectileCount() const { return projectileCount_; }

bool Cannon::canShoot() const { return projectileCount_ > 0; }

void Cannon::decrementProjectileCount() {
  if (projectileCount_ > 0) {
    projectileCount_--;
    updateCounterText();
  }
}

void Cannon::setProjectileCount(unsigned int count) {
  projectileCount_ = count;
  updateCounterText();
}

bool Cannon::shoot(sf::Vector2f &spawnPosition, sf::Vector2f &velocity) {
  if (!canShoot()) {
    return false;
  }

  // Get shooting direction from cannon angle
  sf::Vector2f direction = getShootDirection();

  // Calculate spawn position (at barrel end)
  // Barrel attaches at top center of base, then extends forward by
  // BARREL_LENGTH
  sf::Vector2f barrelOrigin = position_ + sf::Vector2f(0, -BASE_HEIGHT / 2.0f);
  spawnPosition =
      barrelOrigin + direction * (BARREL_LENGTH + BARREL_MUZZLE_LENGTH);

  // Calculate initial velocity (direction * speed)
  constexpr float PROJECTILE_SPEED = 600.0f; // pixels per second
  velocity = direction * PROJECTILE_SPEED;

  // Decrement projectile count
  decrementProjectileCount();

  // Trigger shooting visual effects
  isShooting_ = true;
  shootingEffectTimer_ = SHOOTING_EFFECT_DURATION;

  // Play shooting sound
  AudioManager::getInstance().playSound("shoot");

  barrelRedPulseAlpha_ = 1.0f;
  lightningGlowAlpha_ = 1.0f;
  flameAnimationTime_ = 0.0f; // Reset flame animation

  return true;
}

void Cannon::updateCounterText() {
  std::stringstream ss;
  ss << "x=" << projectileCount_;
  counterText_.setString(ss.str());

  // Center text horizontally
  sf::FloatRect textBounds = counterText_.getLocalBounds();
  counterText_.setOrigin(
      sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
}

void Cannon::clampAngle() {
  angle_ = std::max(MIN_ANGLE, std::min(MAX_ANGLE, angle_));
}

sf::Vector2f Cannon::angleToDirection(float angle) const {
  // Convert angle to direction vector
  // 0° = straight up (0, -1)
  // 45° = up-right (0.707, -0.707)
  // -45° = up-left (-0.707, -0.707)
  float radians = angle * static_cast<float>(M_PI) / 180.0f;
  return sf::Vector2f(std::sin(radians), -std::cos(radians));
}

void Cannon::updateAngleFromMouse(const sf::RenderWindow &window) {
  if (!useMouseControl_) {
    return;
  }

  // Check if window is still open and valid
  if (!window.isOpen()) {
    return;
  }

  try {
    // Get mouse position in window pixel coordinates
    sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);

    // Convert mouse X to angle
    // Map mouse X position to angle: left edge = -45°, center = 0°, right edge
    // = +45°
    float windowWidth = static_cast<float>(window.getSize().x);
    float targetAngle =
        mouseXToAngle(static_cast<float>(mousePixelPos.x), windowWidth);

    // Direct control for responsive aiming (instant response)
    angle_ = targetAngle;

    clampAngle();
  } catch (...) {
    // If there's an error accessing the window, disable mouse control
    useMouseControl_ = false;
  }
}

float Cannon::mouseXToAngle(float mouseX, float windowWidth) const {
  // Convert mouse X position to angle
  // Mouse at left edge (0) = -45°
  // Mouse at center (width/2) = 0°
  // Mouse at right edge (width) = +45°
  float normalizedX = (mouseX / windowWidth) * 2.0f - 1.0f; // -1 to 1
  return normalizedX * MAX_ANGLE * MOUSE_SENSITIVITY;
}

void Cannon::initializeBase() {
  // Main base body (rounded rectangle shape)
  baseBody_.setFillColor(Game::NEON_CYAN);
  baseBody_.setOutlineColor(Game::NEON_CYAN);
  baseBody_.setOutlineThickness(2.0f);
  baseBody_.setOrigin(
      sf::Vector2f(BASE_WIDTH / 2.0f, BASE_HEIGHT)); // Origin at bottom center
  baseBody_.setPosition(position_);

  // Left side panel
  baseLeftPanel_.setFillColor(Game::NEON_PURPLE);
  baseLeftPanel_.setOutlineColor(Game::NEON_CYAN);
  baseLeftPanel_.setOutlineThickness(1.5f);
  baseLeftPanel_.setOrigin(
      sf::Vector2f(BASE_PANEL_WIDTH / 2.0f, BASE_PANEL_HEIGHT));
  baseLeftPanel_.setPosition(
      position_ +
      sf::Vector2f(-BASE_WIDTH / 2.0f - BASE_PANEL_WIDTH / 2.0f, 0));

  // Right side panel
  baseRightPanel_.setFillColor(Game::NEON_PURPLE);
  baseRightPanel_.setOutlineColor(Game::NEON_CYAN);
  baseRightPanel_.setOutlineThickness(1.5f);
  baseRightPanel_.setOrigin(
      sf::Vector2f(BASE_PANEL_WIDTH / 2.0f, BASE_PANEL_HEIGHT));
  baseRightPanel_.setPosition(
      position_ + sf::Vector2f(BASE_WIDTH / 2.0f + BASE_PANEL_WIDTH / 2.0f, 0));

  // Front panel (at top of base)
  baseFrontPanel_.setFillColor(Game::NEON_PINK);
  baseFrontPanel_.setOutlineColor(Game::NEON_CYAN);
  baseFrontPanel_.setOutlineThickness(1.5f);
  baseFrontPanel_.setOrigin(
      sf::Vector2f(BASE_FRONT_WIDTH / 2.0f, BASE_FRONT_HEIGHT));
  baseFrontPanel_.setPosition(
      position_ + sf::Vector2f(0, -BASE_HEIGHT + BASE_FRONT_HEIGHT / 2.0f));

  // Central energy core
  baseCore_.setFillColor(Game::NEON_PINK);
  baseCore_.setOutlineColor(Game::NEON_CYAN);
  baseCore_.setOutlineThickness(1.5f);
  baseCore_.setOrigin(sf::Vector2f(BASE_CORE_RADIUS, BASE_CORE_RADIUS));
  baseCore_.setPosition(position_ + sf::Vector2f(0, -BASE_HEIGHT / 2.0f));
}

void Cannon::initializeBarrel() {
  // Main barrel segment
  barrelMain_.setFillColor(Game::NEON_CYAN);
  barrelMain_.setOutlineColor(Game::NEON_CYAN);
  barrelMain_.setOutlineThickness(2.0f);
  // Origin at bottom center for rotation around attachment point
  barrelMain_.setOrigin(sf::Vector2f(BARREL_LENGTH / 2.0f, BARREL_WIDTH));

  // Muzzle opening (wider at the end)
  barrelMuzzle_.setFillColor(Game::NEON_PURPLE);
  barrelMuzzle_.setOutlineColor(Game::NEON_CYAN);
  barrelMuzzle_.setOutlineThickness(2.0f);
  // Origin at left center (attaches to end of barrel)
  barrelMuzzle_.setOrigin(sf::Vector2f(0, BARREL_MUZZLE_WIDTH / 2.0f));

  // Energy ring 1 (near base of barrel)
  barrelRing1_.setFillColor(sf::Color::Transparent);
  barrelRing1_.setOutlineColor(Game::NEON_PINK);
  barrelRing1_.setOutlineThickness(2.0f);
  barrelRing1_.setOrigin(sf::Vector2f(BARREL_RING_RADIUS, BARREL_RING_RADIUS));

  // Energy ring 2 (middle of barrel)
  barrelRing2_.setFillColor(sf::Color::Transparent);
  barrelRing2_.setOutlineColor(Game::NEON_PINK);
  barrelRing2_.setOutlineThickness(2.0f);
  barrelRing2_.setOrigin(sf::Vector2f(BARREL_RING_RADIUS, BARREL_RING_RADIUS));

  // Top fin
  barrelFin1_.setFillColor(Game::NEON_PURPLE);
  barrelFin1_.setOutlineColor(Game::NEON_CYAN);
  barrelFin1_.setOutlineThickness(1.5f);
  barrelFin1_.setOrigin(
      sf::Vector2f(BARREL_FIN_WIDTH / 2.0f, 0)); // Origin at bottom center

  // Bottom fin
  barrelFin2_.setFillColor(Game::NEON_PURPLE);
  barrelFin2_.setOutlineColor(Game::NEON_CYAN);
  barrelFin2_.setOutlineThickness(1.5f);
  barrelFin2_.setOrigin(sf::Vector2f(
      BARREL_FIN_WIDTH / 2.0f, BARREL_FIN_LENGTH)); // Origin at top center

  // Initial barrel transform
  updateBarrelTransform();
}

void Cannon::updateBarrelTransform() {
  // Calculate rotation origin (top center of base, where barrel attaches)
  // Base origin is at bottom center, so top center is BASE_HEIGHT up
  sf::Vector2f rotationOrigin = position_ + sf::Vector2f(0, -BASE_HEIGHT);

  // Convert angle to radians for calculations
  float angleRad = angle_ * static_cast<float>(M_PI) / 180.0f;
  float cosAngle = std::cos(angleRad);
  float sinAngle = std::sin(angleRad);

  // Main barrel segment (rotates around its bottom center origin)
  barrelMain_.setPosition(rotationOrigin);
  barrelMain_.setRotation(sf::degrees(angle_));

  // Muzzle (at the end of the barrel, extends forward)
  float barrelEndX = sinAngle * BARREL_LENGTH;
  float barrelEndY = -cosAngle * BARREL_LENGTH;
  barrelMuzzle_.setPosition(rotationOrigin +
                            sf::Vector2f(barrelEndX, barrelEndY));
  barrelMuzzle_.setRotation(sf::degrees(angle_));

  // Energy ring 1 (1/3 along barrel)
  float ring1X = sinAngle * (BARREL_LENGTH / 3.0f);
  float ring1Y = -cosAngle * (BARREL_LENGTH / 3.0f);
  barrelRing1_.setPosition(rotationOrigin + sf::Vector2f(ring1X, ring1Y));

  // Energy ring 2 (2/3 along barrel)
  float ring2X = sinAngle * (BARREL_LENGTH * 2.0f / 3.0f);
  float ring2Y = -cosAngle * (BARREL_LENGTH * 2.0f / 3.0f);
  barrelRing2_.setPosition(rotationOrigin + sf::Vector2f(ring2X, ring2Y));

  // Top fin (attached to top side of barrel, at midpoint)
  float finMidX = sinAngle * (BARREL_LENGTH / 2.0f);
  float finMidY = -cosAngle * (BARREL_LENGTH / 2.0f);
  // Offset perpendicular to barrel (to the left/right side)
  float finOffsetX =
      -cosAngle * (BARREL_WIDTH / 2.0f + BARREL_FIN_LENGTH / 2.0f);
  float finOffsetY =
      -sinAngle * (BARREL_WIDTH / 2.0f + BARREL_FIN_LENGTH / 2.0f);
  barrelFin1_.setPosition(rotationOrigin + sf::Vector2f(finMidX + finOffsetX,
                                                        finMidY + finOffsetY));
  barrelFin1_.setRotation(sf::degrees(angle_));

  // Bottom fin (attached to bottom side of barrel, at midpoint)
  // Offset to the opposite side
  float fin2OffsetX =
      cosAngle * (BARREL_WIDTH / 2.0f + BARREL_FIN_LENGTH / 2.0f);
  float fin2OffsetY =
      sinAngle * (BARREL_WIDTH / 2.0f + BARREL_FIN_LENGTH / 2.0f);
  barrelFin2_.setPosition(rotationOrigin + sf::Vector2f(finMidX + fin2OffsetX,
                                                        finMidY + fin2OffsetY));
  barrelFin2_.setRotation(sf::degrees(angle_));
}

void Cannon::updateAnimations(float deltaTime) {
  // Update core pulse animation
  // Pulse speed controls how fast alpha changes (alpha units per second)
  float pulseSpeed =
      (CORE_PULSE_MAX_ALPHA - CORE_PULSE_MIN_ALPHA) * CORE_PULSE_SPEED;
  corePulseAlpha_ += corePulseDirection_ * pulseSpeed * deltaTime;

  // Clamp and reverse direction
  if (corePulseAlpha_ >= CORE_PULSE_MAX_ALPHA) {
    corePulseAlpha_ = CORE_PULSE_MAX_ALPHA;
    corePulseDirection_ = -1.0f; // Start decreasing
  } else if (corePulseAlpha_ <= CORE_PULSE_MIN_ALPHA) {
    corePulseAlpha_ = CORE_PULSE_MIN_ALPHA;
    corePulseDirection_ = 1.0f; // Start increasing
  }

  // Update glow intensity (subtle pulsing synchronized with core)
  // Normalize corePulseAlpha_ to 0-1 range for smooth glow variation
  float normalizedPulse = (corePulseAlpha_ - CORE_PULSE_MIN_ALPHA) /
                          (CORE_PULSE_MAX_ALPHA - CORE_PULSE_MIN_ALPHA);
  glowIntensity_ = GLOW_INTENSITY_MIN +
                   (GLOW_INTENSITY_MAX - GLOW_INTENSITY_MIN) * normalizedPulse;
}

void Cannon::updateShootingEffects(float deltaTime) {
  if (isShooting_) {
    // Update timer
    shootingEffectTimer_ -= deltaTime;

    // Update barrel red pulse (pulsating between 0.6 and 1.0 while shooting for
    // more visible effect) Use sine wave for smooth pulsation
    pulseTime_ += deltaTime * BARREL_RED_PULSE_SPEED;
    barrelRedPulseAlpha_ =
        0.6f +
        0.4f *
            (0.5f + 0.5f * std::sin(pulseTime_)); // Pulse between 0.6 and 1.0

    // Update flame animation time
    flameAnimationTime_ += deltaTime * FLAME_ANIMATION_SPEED;

    // Update lightning glow (fade out)
    lightningGlowAlpha_ -= LIGHTNING_GLOW_SPEED * deltaTime;
    if (lightningGlowAlpha_ < 0.0f) {
      lightningGlowAlpha_ = 0.0f;
    }

    // End shooting effect when timer expires
    if (shootingEffectTimer_ <= 0.0f) {
      isShooting_ = false;
      barrelRedPulseAlpha_ = 0.0f;
      lightningGlowAlpha_ = 0.0f;
      pulseTime_ = 0.0f;
      flameAnimationTime_ = 0.0f;
    }
  } else {
    // Fade out effects if not shooting
    barrelRedPulseAlpha_ = std::max(
        0.0f, barrelRedPulseAlpha_ - BARREL_RED_PULSE_SPEED * deltaTime);
    lightningGlowAlpha_ =
        std::max(0.0f, lightningGlowAlpha_ - LIGHTNING_GLOW_SPEED * deltaTime);
    flameAnimationTime_ = 0.0f;
  }
}

void Cannon::renderGlow(sf::RenderWindow &window,
                        const sf::RectangleShape &shape,
                        const sf::Color &baseColor, float intensity) const {
  // Render multiple glow layers with decreasing opacity and increasing size
  for (int i = 0; i < GLOW_LAYERS; ++i) {
    float layerAlpha = intensity * (40.0f - i * 12.0f); // Decreasing alpha
    float layerScale = 1.0f + (i + 1) * 0.08f;          // Increasing size

    sf::RectangleShape glowLayer(shape.getSize());
    glowLayer.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b,
                                     static_cast<unsigned char>(layerAlpha)));
    glowLayer.setOutlineColor(sf::Color::Transparent);
    glowLayer.setOutlineThickness(0.0f);
    glowLayer.setOrigin(shape.getOrigin());
    glowLayer.setPosition(shape.getPosition());
    glowLayer.setRotation(shape.getRotation());

    // Calculate scale: base scale * layer scale
    sf::Vector2f baseScale = shape.getScale();
    glowLayer.setScale(
        sf::Vector2f(baseScale.x * layerScale, baseScale.y * layerScale));

    window.draw(glowLayer);
  }
}

void Cannon::renderGlow(sf::RenderWindow &window, const sf::CircleShape &shape,
                        const sf::Color &baseColor, float intensity) const {
  // Render multiple glow layers with decreasing opacity and increasing size
  for (int i = 0; i < GLOW_LAYERS; ++i) {
    float layerAlpha = intensity * (40.0f - i * 12.0f); // Decreasing alpha
    float layerScale = 1.0f + (i + 1) * 0.08f;          // Increasing size

    sf::CircleShape glowLayer(shape.getRadius());
    glowLayer.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b,
                                     static_cast<unsigned char>(layerAlpha)));
    glowLayer.setOutlineColor(sf::Color::Transparent);
    glowLayer.setOutlineThickness(0.0f);
    glowLayer.setOrigin(shape.getOrigin());
    glowLayer.setPosition(shape.getPosition());
    glowLayer.setRotation(shape.getRotation());

    // Calculate scale: base scale * layer scale
    sf::Vector2f baseScale = shape.getScale();
    glowLayer.setScale(
        sf::Vector2f(baseScale.x * layerScale, baseScale.y * layerScale));

    window.draw(glowLayer);
  }
}

void Cannon::renderLightningGlow(sf::RenderWindow &window) const {
  if (lightningGlowAlpha_ <= 0.0f) {
    return;
  }

  // Render white lightning glow on cannon boundaries
  // Create outline shapes with white glow
  unsigned char alpha =
      static_cast<unsigned char>(lightningGlowAlpha_ * 255.0f);

  // Render glow on base body outline
  sf::RectangleShape baseGlow(
      sf::Vector2f(BASE_WIDTH + 4.0f, BASE_HEIGHT + 4.0f));
  baseGlow.setOrigin(
      sf::Vector2f((BASE_WIDTH + 4.0f) / 2.0f, (BASE_HEIGHT + 4.0f) / 2.0f));
  baseGlow.setPosition(baseBody_.getPosition());
  baseGlow.setFillColor(sf::Color::Transparent);
  baseGlow.setOutlineColor(sf::Color(LIGHTNING_WHITE.r, LIGHTNING_WHITE.g,
                                     LIGHTNING_WHITE.b, alpha));
  baseGlow.setOutlineThickness(3.0f);
  window.draw(baseGlow);

  // Render glow on barrel outline
  sf::RectangleShape barrelGlow(
      sf::Vector2f(BARREL_LENGTH + 4.0f, BARREL_WIDTH + 4.0f));
  barrelGlow.setOrigin(sf::Vector2f(0.0f, (BARREL_WIDTH + 4.0f) / 2.0f));
  barrelGlow.setPosition(barrelMain_.getPosition());
  barrelGlow.setRotation(barrelMain_.getRotation());
  barrelGlow.setFillColor(sf::Color::Transparent);
  barrelGlow.setOutlineColor(sf::Color(LIGHTNING_WHITE.r, LIGHTNING_WHITE.g,
                                       LIGHTNING_WHITE.b, alpha));
  barrelGlow.setOutlineThickness(2.5f);
  window.draw(barrelGlow);

  // Render glow on side panels
  sf::RectangleShape leftPanelGlow(
      sf::Vector2f(BASE_PANEL_WIDTH + 3.0f, BASE_PANEL_HEIGHT + 3.0f));
  leftPanelGlow.setOrigin(sf::Vector2f((BASE_PANEL_WIDTH + 3.0f) / 2.0f,
                                       (BASE_PANEL_HEIGHT + 3.0f) / 2.0f));
  leftPanelGlow.setPosition(baseLeftPanel_.getPosition());
  leftPanelGlow.setFillColor(sf::Color::Transparent);
  leftPanelGlow.setOutlineColor(sf::Color(LIGHTNING_WHITE.r, LIGHTNING_WHITE.g,
                                          LIGHTNING_WHITE.b, alpha));
  leftPanelGlow.setOutlineThickness(2.0f);
  window.draw(leftPanelGlow);

  sf::RectangleShape rightPanelGlow(
      sf::Vector2f(BASE_PANEL_WIDTH + 3.0f, BASE_PANEL_HEIGHT + 3.0f));
  rightPanelGlow.setOrigin(sf::Vector2f((BASE_PANEL_WIDTH + 3.0f) / 2.0f,
                                        (BASE_PANEL_HEIGHT + 3.0f) / 2.0f));
  rightPanelGlow.setPosition(baseRightPanel_.getPosition());
  rightPanelGlow.setFillColor(sf::Color::Transparent);
  rightPanelGlow.setOutlineColor(sf::Color(LIGHTNING_WHITE.r, LIGHTNING_WHITE.g,
                                           LIGHTNING_WHITE.b, alpha));
  rightPanelGlow.setOutlineThickness(2.0f);
  window.draw(rightPanelGlow);
}

void Cannon::renderFlameEffect(sf::RenderWindow &window) const {
  if (!isShooting_ || shootingEffectTimer_ <= 0.0f) {
    return;
  }

  // Calculate flame intensity based on remaining shooting effect time
  float flameIntensity = shootingEffectTimer_ / SHOOTING_EFFECT_DURATION;

  // Convert angle to radians for calculations
  float angleRad = angle_ * static_cast<float>(M_PI) / 180.0f;
  float cosAngle = std::cos(angleRad);
  float sinAngle = std::sin(angleRad);

  // Render flames along base contours
  renderBaseContourFlames(window, flameIntensity);

  // Render flames along barrel contours
  renderBarrelContourFlames(window, flameIntensity, cosAngle, sinAngle);
}

void Cannon::renderBaseContourFlames(sf::RenderWindow &window,
                                     float intensity) const {
  // Render flames along base body contours (rectangular base)
  renderShapeContourFlames(window, baseBody_, intensity);

  // Render flames along left panel contours
  renderShapeContourFlames(window, baseLeftPanel_, intensity);

  // Render flames along right panel contours
  renderShapeContourFlames(window, baseRightPanel_, intensity);

  // Render flames along front panel contours
  renderShapeContourFlames(window, baseFrontPanel_, intensity);
}

void Cannon::renderBarrelContourFlames(sf::RenderWindow &window,
                                       float intensity, float /* cosAngle */,
                                       float /* sinAngle */) const {
  // Render flames along barrel main contours
  renderShapeContourFlames(window, barrelMain_, intensity);

  // Render flames along barrel muzzle contours
  renderShapeContourFlames(window, barrelMuzzle_, intensity);

  // Render flames along barrel fins
  renderShapeContourFlames(window, barrelFin1_, intensity);
  renderShapeContourFlames(window, barrelFin2_, intensity);
}

void Cannon::renderShapeContourFlames(sf::RenderWindow &window,
                                      const sf::RectangleShape &shape,
                                      float intensity) const {
  // Get shape properties
  sf::Vector2f size = shape.getSize();
  sf::Vector2f position = shape.getPosition();
  sf::Vector2f origin = shape.getOrigin();
  float rotation = shape.getRotation().asDegrees();

  // Calculate corners relative to origin (before rotation)
  // Origin is relative to the shape's local coordinate system
  // Corners are: top-left, top-right, bottom-right, bottom-left
  sf::Vector2f localCorners[4] = {
      sf::Vector2f(-origin.x, -origin.y), // Top-left (relative to origin)
      sf::Vector2f(size.x - origin.x, -origin.y),         // Top-right
      sf::Vector2f(size.x - origin.x, size.y - origin.y), // Bottom-right
      sf::Vector2f(-origin.x, size.y - origin.y)          // Bottom-left
  };

  // Apply rotation and transform to world coordinates
  float angleRad = rotation * static_cast<float>(M_PI) / 180.0f;
  float cosRot = std::cos(angleRad);
  float sinRot = std::sin(angleRad);

  sf::Vector2f worldCorners[4];
  for (int i = 0; i < 4; ++i) {
    // Rotate corner around origin, then translate to world position
    float x = localCorners[i].x;
    float y = localCorners[i].y;
    worldCorners[i] = sf::Vector2f(x * cosRot - y * sinRot + position.x,
                                   x * sinRot + y * cosRot + position.y);
  }

  // Render flames along each edge
  for (int i = 0; i < 4; ++i) {
    int next = (i + 1) % 4;
    renderEdgeFlames(window, worldCorners[i], worldCorners[next], intensity,
                     false);
  }
}

void Cannon::renderEdgeFlames(sf::RenderWindow &window,
                              const sf::Vector2f &start,
                              const sf::Vector2f &end, float intensity,
                              bool /* isBarrel */) const {
  // Calculate edge direction and length
  sf::Vector2f edgeDir = end - start;
  float edgeLength = std::sqrt(edgeDir.x * edgeDir.x + edgeDir.y * edgeDir.y);

  if (edgeLength < 0.1f) {
    return; // Skip zero-length edges
  }

  // Normalize edge direction
  edgeDir.x /= edgeLength;
  edgeDir.y /= edgeLength;

  // Calculate perpendicular direction (for flame offset)
  sf::Vector2f perpDir(-edgeDir.y, edgeDir.x);

  // Calculate number of flame points along edge
  int numFlames = static_cast<int>(edgeLength / CONTOUR_FLAME_SPACING) + 1;

  // Render flames along the edge
  for (int i = 0; i < numFlames; ++i) {
    float t = (numFlames > 1)
                  ? (static_cast<float>(i) / static_cast<float>(numFlames - 1))
                  : 0.0f;
    sf::Vector2f edgePos = start + edgeDir * (edgeLength * t);

    // Add some variation based on animation time
    float flameTime = flameAnimationTime_ + static_cast<float>(i) * 0.3f;
    float flickerOffset = 1.5f * std::sin(flameTime * 4.0f) * intensity;
    float sizeVariation =
        0.3f * std::sin(flameTime * 6.0f) + 0.2f * std::sin(flameTime * 9.0f);

    // Calculate flame position (slightly offset outward from edge)
    sf::Vector2f flamePos = edgePos + perpDir * flickerOffset;

    // Calculate flame size with variation
    float flameSize = CONTOUR_FLAME_SIZE * (1.0f + sizeVariation) * intensity;

    // Render multiple flame layers for each point
    for (int layer = 0; layer < CONTOUR_FLAME_POINTS; ++layer) {
      float layerOffset =
          static_cast<float>(layer) / static_cast<float>(CONTOUR_FLAME_POINTS);
      float layerTime = flameTime + layerOffset * 1.5f;

      // Calculate flame color (orange to red gradient)
      sf::Color flameColor;
      if (layerOffset < 0.5f) {
        // Inner: orange
        float t = layerOffset / 0.5f;
        flameColor =
            sf::Color(static_cast<unsigned char>(FLAME_MID.r * (1.0f - t) +
                                                 FLAME_OUTER.r * t),
                      static_cast<unsigned char>(FLAME_MID.g * (1.0f - t) +
                                                 FLAME_OUTER.g * t),
                      static_cast<unsigned char>(FLAME_MID.b * (1.0f - t) +
                                                 FLAME_OUTER.b * t));
      } else {
        // Outer: red-orange with fade
        float t = (layerOffset - 0.5f) / 0.5f;
        flameColor = FLAME_OUTER;
        flameColor.a =
            static_cast<unsigned char>(255 * (1.0f - t * 0.6f) * intensity);
      }

      // Apply intensity-based alpha
      flameColor.a = static_cast<unsigned char>(flameColor.a * intensity *
                                                (1.0f - layerOffset * 0.4f));

      // Calculate layer-specific position offset
      float layerFlicker =
          1.0f * std::sin(layerTime * 5.0f) * (1.0f - layerOffset);
      sf::Vector2f layerPos = flamePos + perpDir * layerFlicker;

      // Calculate layer-specific size
      float layerSize = flameSize * (1.0f - layerOffset * 0.3f);

      // Render flame as small circle/particle
      sf::CircleShape flame(layerSize);
      flame.setOrigin(sf::Vector2f(layerSize, layerSize));
      flame.setPosition(layerPos);
      flame.setFillColor(flameColor);
      flame.setOutlineThickness(0.0f);

      window.draw(flame);
    }
  }
}
