#include "entities/Projectile.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>

// ============================================================================
// Projectile Implementation
// ============================================================================

Projectile::Projectile()
    : position_(0.0f, 0.0f), velocity_(0.0f, 0.0f), isActive_(false),
      shape_(PROJECTILE_RADIUS), trailUpdateTimer_(0.0f) {
  shape_.setFillColor(PROJECTILE_COLOR);
  shape_.setOutlineColor(sf::Color(255, 255, 255, 200));
  shape_.setOutlineThickness(1.0f);
  shape_.setOrigin(sf::Vector2f(PROJECTILE_RADIUS, PROJECTILE_RADIUS));
  shape_.setPosition(position_);
}

void Projectile::activate(const sf::Vector2f &position,
                          const sf::Vector2f &velocity) {
  position_ = position;
  velocity_ = velocity;
  isActive_ = true;
  trailPoints_.clear();
  trailUpdateTimer_ = 0.0f;

  // Add initial trail point
  trailPoints_.push_back({position_, 1.0f, 0.0f});

  shape_.setPosition(position_);
}

void Projectile::deactivate() {
  isActive_ = false;
  trailPoints_.clear();
}

void Projectile::update(float deltaTime, const sf::Vector2u &windowSize,
                        const sf::FloatRect &cannonBounds) {
  if (!isActive_) {
    return;
  }

  // Update position
  position_ += velocity_ * deltaTime;
  shape_.setPosition(position_);

  // Check collisions
  checkWallCollisions(windowSize);
  checkCannonCollision(cannonBounds);

  // Check if projectile left screen at bottom
  if (position_.y > windowSize.y + PROJECTILE_RADIUS) {
    deactivate();
    return;
  }

  // Update trail
  updateTrail(deltaTime);
}

void Projectile::updateTrail(float deltaTime) {
  // Update trail timer
  trailUpdateTimer_ += deltaTime;

  // Add new trail point periodically
  if (trailUpdateTimer_ >= TRAIL_UPDATE_INTERVAL) {
    trailUpdateTimer_ = 0.0f;

    // Add new trail point
    trailPoints_.push_back({position_, 1.0f, 0.0f});

    // Limit trail points
    if (trailPoints_.size() > MAX_TRAIL_POINTS) {
      trailPoints_.pop_front();
    }
  }

  // Update existing trail points
  for (auto &point : trailPoints_) {
    point.age += deltaTime;
    point.alpha = std::max(0.0f, 1.0f - (point.age * TRAIL_FADE_SPEED));
  }

  // Remove expired trail points
  trailPoints_.erase(std::remove_if(trailPoints_.begin(), trailPoints_.end(),
                                    [](const TrailPoint &point) {
                                      return point.alpha <= 0.0f;
                                    }),
                     trailPoints_.end());
}

void Projectile::checkWallCollisions(const sf::Vector2u &windowSize) {
  // Left wall
  if (position_.x - PROJECTILE_RADIUS < 0) {
    position_.x = PROJECTILE_RADIUS + COLLISION_OFFSET;
    velocity_.x = -velocity_.x;
  }

  // Right wall
  if (position_.x + PROJECTILE_RADIUS > windowSize.x) {
    position_.x = windowSize.x - PROJECTILE_RADIUS - COLLISION_OFFSET;
    velocity_.x = -velocity_.x;
  }

  // Top wall
  if (position_.y - PROJECTILE_RADIUS < 0) {
    position_.y = PROJECTILE_RADIUS + COLLISION_OFFSET;
    velocity_.y = -velocity_.y;
  }
}

void Projectile::checkCannonCollision(const sf::FloatRect &cannonBounds) {
  sf::FloatRect projectileBounds = getBounds();

  // Manual AABB intersection check (SFML 3.0 doesn't have intersects() method)
  bool intersects = (projectileBounds.position.x <
                         cannonBounds.position.x + cannonBounds.size.x &&
                     projectileBounds.position.x + projectileBounds.size.x >
                         cannonBounds.position.x &&
                     projectileBounds.position.y <
                         cannonBounds.position.y + cannonBounds.size.y &&
                     projectileBounds.position.y + projectileBounds.size.y >
                         cannonBounds.position.y);

  if (intersects) {
    // Simple bounce: always bounce upward
    velocity_.y = -std::abs(velocity_.y);

    // Offset projectile to prevent sticking
    position_.y =
        cannonBounds.position.y - PROJECTILE_RADIUS - COLLISION_OFFSET;
    shape_.setPosition(position_);
  }
}

void Projectile::render(sf::RenderWindow &window) const {
  if (!isActive_) {
    return;
  }

  // Render trail first (behind projectile)
  renderTrail(window);

  // Render glow layers
  renderGlow(window);

  // Render main projectile
  window.draw(shape_);
}

void Projectile::renderGlow(sf::RenderWindow &window) const {
  // Render multiple glow layers with decreasing opacity
  for (int i = 0; i < GLOW_LAYERS; ++i) {
    float scale = 1.0f + (static_cast<float>(i + 1) * 0.08f);
    unsigned char alpha = static_cast<unsigned char>(40 - (i * 12));

    sf::CircleShape glow(PROJECTILE_RADIUS * scale);
    glow.setOrigin(
        sf::Vector2f(PROJECTILE_RADIUS * scale, PROJECTILE_RADIUS * scale));
    glow.setPosition(position_);
    glow.setFillColor(sf::Color(PROJECTILE_COLOR.r, PROJECTILE_COLOR.g,
                                PROJECTILE_COLOR.b, alpha));
    glow.setOutlineThickness(0.0f);

    window.draw(glow);
  }
}

void Projectile::renderTrail(sf::RenderWindow &window) const {
  if (trailPoints_.size() < 2) {
    return;
  }

  // Render trail as connected lines
  for (size_t i = 0; i < trailPoints_.size() - 1; ++i) {
    const auto &point1 = trailPoints_[i];
    const auto &point2 = trailPoints_[i + 1];

    // Calculate line thickness based on alpha
    float thickness = 2.0f * point1.alpha;

    // Create line segment
    sf::Vector2f direction = point2.position - point1.position;
    float length =
        std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (length > 0.0f) {
      sf::RectangleShape line(sf::Vector2f(length, thickness));
      line.setOrigin(sf::Vector2f(0.0f, thickness / 2.0f));
      line.setPosition(point1.position);

      // Calculate rotation angle
      float angle = std::atan2(direction.y, direction.x) * 180.0f /
                    static_cast<float>(M_PI);
      line.setRotation(sf::degrees(angle));

      // Set color with alpha
      unsigned char alpha = static_cast<unsigned char>(255 * point1.alpha);
      line.setFillColor(sf::Color(PROJECTILE_COLOR.r, PROJECTILE_COLOR.g,
                                  PROJECTILE_COLOR.b, alpha));

      window.draw(line);
    }
  }
}

sf::Vector2f Projectile::getPosition() const { return position_; }

sf::Vector2f Projectile::getVelocity() const { return velocity_; }

sf::FloatRect Projectile::getBounds() const {
  // SFML 3.0: FloatRect constructor takes position (Vector2f) and size
  // (Vector2f)
  return sf::FloatRect(
      sf::Vector2f(position_.x - PROJECTILE_RADIUS,
                   position_.y - PROJECTILE_RADIUS),
      sf::Vector2f(PROJECTILE_RADIUS * 2.0f, PROJECTILE_RADIUS * 2.0f));
}

bool Projectile::isActive() const { return isActive_; }

void Projectile::setVelocity(const sf::Vector2f &velocity) {
  velocity_ = velocity;
}

void Projectile::setPosition(const sf::Vector2f &position) {
  position_ = position;
  shape_.setPosition(position_);
}

// ============================================================================
// ProjectilePool Implementation
// ============================================================================

ProjectilePool::ProjectilePool(size_t poolSize)
    : pool_(poolSize), activeCount_(0) {}

Projectile *ProjectilePool::acquire(const sf::Vector2f &position,
                                    const sf::Vector2f &velocity) {
  // Find first inactive projectile
  for (auto &projectile : pool_) {
    if (!projectile.isActive()) {
      projectile.activate(position, velocity);
      activeCount_++;
      return &projectile;
    }
  }

  // Pool is full
  return nullptr;
}

void ProjectilePool::updateAll(float deltaTime, const sf::Vector2u &windowSize,
                               const sf::FloatRect &cannonBounds) {
  activeCount_ = 0;
  for (auto &projectile : pool_) {
    if (projectile.isActive()) {
      projectile.update(deltaTime, windowSize, cannonBounds);
      if (projectile.isActive()) {
        activeCount_++;
      }
    }
  }
}

void ProjectilePool::renderAll(sf::RenderWindow &window) const {
  for (const auto &projectile : pool_) {
    if (projectile.isActive()) {
      projectile.render(window);
    }
  }
}

size_t ProjectilePool::getActiveCount() const { return activeCount_; }

std::vector<Projectile *> ProjectilePool::getActiveProjectiles() {
  std::vector<Projectile *> activeProjectiles;
  for (auto &projectile : pool_) {
    if (projectile.isActive()) {
      activeProjectiles.push_back(&projectile);
    }
  }
  return activeProjectiles;
}

std::vector<const Projectile *> ProjectilePool::getActiveProjectiles() const {
  std::vector<const Projectile *> activeProjectiles;
  for (const auto &projectile : pool_) {
    if (projectile.isActive()) {
      activeProjectiles.push_back(&projectile);
    }
  }
  return activeProjectiles;
}

void ProjectilePool::clear() {
  for (auto &projectile : pool_) {
    if (projectile.isActive()) {
      projectile.deactivate();
    }
  }
  activeCount_ = 0;
}
