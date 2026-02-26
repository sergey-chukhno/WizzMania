#include "core/states/PlayingState.h"
#include "core/FontManager.h"
#include "core/Game.h"
#include "core/states/GameOverState.h"
#include "core/states/MenuState.h"
#include "core/states/PausedState.h"
#include "entities/Brick.h"
#include "entities/Cannon.h"
#include "entities/Projectile.h"
#include "managers/BlockManager.h"
#include "ui/Starfield.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PlayingState::PlayingState(Game *game)
    : game_(game), font_(FontManager::getBodyFont()),
      starfield_(nullptr) // Will be initialized in onEnter()
      ,
      projectilePool_(100) // Pool size: 100 projectiles
      ,
      currentLevel_(1) // Will be synced with BlockManager in onEnter()
      ,
      score_(0), displayedScore_(0), bricksDestroyed_(0), highScore_(0),
      scoreText_(font_, "Score: 0", HUD_SCORE_FONT_SIZE),
      levelText_(font_, "Level: 1", HUD_LEVEL_FONT_SIZE),
      highScoreText_(font_, "", HUD_HIGH_SCORE_FONT_SIZE),
      hudAnimationTime_(0.0f), scoreGlowIntensity_(HUD_GLOW_INTENSITY_MAX),
      levelGlowIntensity_(HUD_GLOW_INTENSITY_MAX),
      highScoreGlowIntensity_(HUD_GLOW_INTENSITY_MAX), levelChangeFlash_(0.0f),
      highScoreFlash_(0.0f) {
  // Initialize score text
  scoreText_.setFillColor(getHUDScoreColor());
  scoreText_.setStyle(sf::Text::Bold);

  // Initialize level text
  levelText_.setFillColor(getHUDLevelColor());
  levelText_.setStyle(sf::Text::Bold);

  // Initialize high score text
  highScoreText_.setFillColor(getHUDHighScoreColor());
  highScoreText_.setStyle(sf::Text::Bold);

  // Load high score
  loadHighScore();
}

void PlayingState::update(float deltaTime) {
  // Update starfield background
  if (starfield_) {
    starfield_->update(deltaTime);
  }

  // Update cannon
  if (cannon_) {
    cannon_->update(deltaTime, game_->getWindow());

    // Update all projectiles
    sf::Vector2u windowSize(game_->getWindowWidth(), game_->getWindowHeight());
    sf::FloatRect cannonBounds = cannon_->getBounds();
    projectilePool_.updateAll(deltaTime, windowSize, cannonBounds);

    // Update BlockManager
    if (blockManager_) {
      blockManager_->update(deltaTime, cannonBounds);

      // Check collisions between projectiles and bricks (this may destroy
      // bricks)
      checkProjectileBrickCollisions();

      // Update block destroyed states AFTER collision detection
      // This ensures blocks are marked as destroyed when all their bricks are
      // destroyed
      blockManager_->updateBlockDestroyedStates(deltaTime);

      // Check level completion FIRST (before game over)
      // This ensures we can advance to next level even if blocks are near the
      // bottom
      if (blockManager_->isLevelComplete()) {
        // Advance to next level
        // advanceLevel() will increment the level internally
        blockManager_->advanceLevel();
        // Update currentLevel_ to match BlockManager's level
        currentLevel_ = blockManager_->getCurrentLevel();

        // Reset projectile count to 50 for the new level
        if (cannon_) {
          cannon_->setProjectileCount(50);
        }

        // Trigger level change flash effect
        triggerLevelChangeFlash();

        std::cout << "Level " << currentLevel_ << " started" << std::endl;
        // Continue to next frame - don't check game over if level is complete
        return;
      }

      // Check game over conditions (only if level is not complete)
      if (blockManager_->hasBlocksReachedBottom() ||
          blockManager_->hasBlocksTouchedCannon(cannonBounds)) {
        // Update high score if needed
        if (score_ > highScore_) {
          highScore_ = score_;
          saveHighScore();
          triggerHighScoreFlash();
        }

        // Trigger game over with statistics
        game_->queueStateChange(std::make_unique<GameOverState>(
            game_, score_, currentLevel_, bricksDestroyed_));
        return;
      }
    }

    // Update explosion particles
    updateExplosionParticles(deltaTime);

    // Update animated score display
    updateScoreDisplay(deltaTime);

    // Update HUD animations (glow effects, flashes)
    updateHUDAnimations(deltaTime);

    // Update level display when level changes
    updateLevelDisplay();

    // Update high score display
    updateHighScoreDisplay();
  }
}

void PlayingState::render(sf::RenderWindow &window) {
  // Render starfield background first
  if (starfield_) {
    starfield_->render(window);
  }

  // Render blocks (through BlockManager, behind projectiles and cannon)
  if (blockManager_) {
    blockManager_->render(window);
  }

  // Render projectiles (behind cannon)
  projectilePool_.renderAll(window);

  // Render explosion particles (behind cannon, on top of projectiles)
  renderExplosionParticles(window);

  // Render cannon
  if (cannon_) {
    cannon_->render(window);
  }

  // Render HUD elements with strong cyberpunk glow effects
  // Score (top-left)
  renderTextWithGlow(window, scoreText_, getHUDScoreColor(),
                     scoreGlowIntensity_);

  // Level (below score, top-left)
  renderTextWithGlow(
      window, levelText_, getHUDLevelColor(), levelGlowIntensity_,
      levelChangeFlash_ > 0.0f ? 1.0f + levelChangeFlash_ * 0.5f : 1.0f);

  // High score (top-right, only if > 0)
  if (highScore_ > 0) {
    renderTextWithGlow(
        window, highScoreText_, getHUDHighScoreColor(), highScoreGlowIntensity_,
        highScoreFlash_ > 0.0f ? 1.0f + highScoreFlash_ * 0.5f : 1.0f);
  }
}

void PlayingState::handleEvent(const sf::Event &event) {
  // Pass events to cannon for angle control
  if (cannon_) {
    cannon_->handleInput(event, game_->getWindow());
  }

  // Handle shooting (Space bar or mouse click)
  if (cannon_ && cannon_->canShoot()) {
    bool shootRequested = false;

    if (auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
      if (keyPressed->code == sf::Keyboard::Key::Space) {
        shootRequested = true;
      }
    }

    if (auto *mouseButton = event.getIf<sf::Event::MouseButtonPressed>()) {
      if (mouseButton->button == sf::Mouse::Button::Left) {
        shootRequested = true;
      }
    }

    if (shootRequested) {
      // Shoot projectile from cannon
      sf::Vector2f spawnPosition;
      sf::Vector2f velocity;
      if (cannon_->shoot(spawnPosition, velocity)) {
        // Acquire projectile from pool
        Projectile *projectile =
            projectilePool_.acquire(spawnPosition, velocity);
        if (!projectile) {
          std::cerr << "Warning: Projectile pool is full!" << std::endl;
        }
      }
    }
  }

  // Handle game state transitions
  if (auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    // P key: Pause
    if (keyPressed->code == sf::Keyboard::Key::P) {
      std::cout << "Pause key pressed" << std::endl;
      game_->pushState(std::make_unique<PausedState>(game_));
    }
    // ESC key: Return to menu
    else if (keyPressed->code == sf::Keyboard::Key::Escape) {
      std::cout << "Returning to menu" << std::endl;
      game_->queueStateChange(std::make_unique<MenuState>(game_));
    }
  }
}

void PlayingState::onEnter() {
  std::cout << "Entered PlayingState" << std::endl;

  try {
    // Clear projectile hit tracking
    projectileHitBricks_.clear();

    // Initialize cannon at bottom center
    float cannonX = static_cast<float>(game_->getWindowWidth()) / 2.0f;
    float cannonY = static_cast<float>(game_->getWindowHeight()) -
                    50.0f; // 50px from bottom
    std::cout << "Creating cannon at (" << cannonX << ", " << cannonY << ")"
              << std::endl;

    cannon_ = std::make_unique<Cannon>(sf::Vector2f(cannonX, cannonY),
                                       50 // Initial projectile count
    );

    std::cout << "Cannon created successfully" << std::endl;

    // Initialize BlockManager
    blockManager_ = std::make_unique<BlockManager>(game_->getWindowWidth(),
                                                   game_->getWindowHeight());

    // Initialize starfield background
    starfield_ = std::make_unique<Starfield>(
        150, // Star count for gameplay (slightly less than menu for
             // performance)
        sf::Vector2u(game_->getWindowWidth(), game_->getWindowHeight()));

    // Initialize game state
    score_ = 0;
    displayedScore_ = 0;
    bricksDestroyed_ = 0;

    // Initialize HUD displays
    initializeScoreDisplay();
    initializeLevelDisplay();
    initializeHighScoreDisplay();

    // Reset HUD animation state
    hudAnimationTime_ = 0.0f;
    scoreGlowIntensity_ = HUD_GLOW_INTENSITY_MAX;
    levelGlowIntensity_ = HUD_GLOW_INTENSITY_MAX;
    highScoreGlowIntensity_ = HUD_GLOW_INTENSITY_MAX;
    levelChangeFlash_ = 0.0f;
    highScoreFlash_ = 0.0f;

    // Start level 1
    blockManager_->startLevel(1);
    // Sync currentLevel_ with BlockManager's level
    currentLevel_ = blockManager_->getCurrentLevel();
    std::cout << "Level " << currentLevel_ << " started" << std::endl;

    // Initialize IPC
    sharedMemory_ =
        std::make_unique<wizz::NativeSharedMemory>(wizz::SHARED_MEMORY_KEY);
    if (sharedMemory_->createAndMap() || sharedMemory_->openAndMap()) {
      sharedMemory_->lock();
      if (sharedMemory_->data()) {
        sharedMemory_->data()->isPlaying = true;
        sharedMemory_->data()->currentScore = score_;
        std::strncpy(sharedMemory_->data()->gameName, "BrickBreaker",
                     sizeof(sharedMemory_->data()->gameName) - 1);
        sharedMemory_->data()
            ->gameName[sizeof(sharedMemory_->data()->gameName) - 1] = '\0';
      }
      sharedMemory_->unlock();
      std::cout << "BrickBreaker IPC connected successfully" << std::endl;
    } else {
      std::cerr << "Failed to initialize Shared Memory IPC for BrickBreaker"
                << std::endl;
    }

    std::cout << "PlayingState initialization complete" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error in PlayingState::onEnter(): " << e.what() << std::endl;
    throw;
  } catch (...) {
    std::cerr << "Unknown error in PlayingState::onEnter()" << std::endl;
    throw;
  }
}

void PlayingState::onExit() {
  if (sharedMemory_) {
    sharedMemory_->lock();
    if (sharedMemory_->data()) {
      sharedMemory_->data()->isPlaying = false;
    }
    sharedMemory_->unlock();
    sharedMemory_->close();
  }
  std::cout << "Exited PlayingState" << std::endl;
}

void PlayingState::checkProjectileBrickCollisions() {
  if (!blockManager_) {
    return;
  }

  // Get all active projectiles and blocks
  std::vector<Projectile *> activeProjectiles =
      projectilePool_.getActiveProjectiles();
  std::vector<Block *> activeBlocks = blockManager_->getActiveBlocks();

  // Clean up hit tracking for deactivated projectiles
  for (auto it = projectileHitBricks_.begin();
       it != projectileHitBricks_.end();) {
    Projectile *proj = it->first;
    if (!proj || !proj->isActive()) {
      it = projectileHitBricks_.erase(it);
    } else {
      ++it;
    }
  }

  // Iterate through all active projectiles
  for (Projectile *projectile : activeProjectiles) {
    if (!projectile || !projectile->isActive()) {
      continue;
    }

    sf::FloatRect projectileBounds = projectile->getBounds();
    bool hasCollided =
        false; // Track if projectile has collided (for bounce direction)
    sf::FloatRect firstCollisionBounds; // Store first collision for bounce

    // Get or create the list of bricks this projectile has hit
    auto &hitBricks = projectileHitBricks_[projectile];

    // Clean up hit tracking: remove bricks that are no longer overlapping
    // This allows the projectile to hit the same brick again if it moves away
    // and comes back
    hitBricks.erase(std::remove_if(hitBricks.begin(), hitBricks.end(),
                                   [&projectileBounds, this](Brick *brick) {
                                     if (!brick || brick->isDestroyed()) {
                                       return true; // Remove destroyed bricks
                                     }
                                     // Remove if no longer overlapping
                                     // (projectile has moved away)
                                     return !checkAABBCollision(
                                         projectileBounds, brick->getBounds());
                                   }),
                    hitBricks.end());

    // For each block
    for (Block *block : activeBlocks) {
      if (!block || block->isDestroyed()) {
        continue;
      }

      // Get all bricks in the block
      std::vector<Brick *> bricks = block->getBricks();

      // For each brick
      for (Brick *brick : bricks) {
        if (!brick || brick->isDestroyed()) {
          continue;
        }

        // Skip if this projectile has already hit this brick and is still
        // overlapping
        bool alreadyHit = std::find(hitBricks.begin(), hitBricks.end(),
                                    brick) != hitBricks.end();
        if (alreadyHit) {
          continue;
        }

        // Get brick bounds
        sf::FloatRect brickBounds = brick->getBounds();

        // Check collision (AABB)
        if (checkAABBCollision(projectileBounds, brickBounds)) {
          // Mark this brick as hit by this projectile
          hitBricks.push_back(brick);

          // Get brick color and position before damage (for explosion effect)
          sf::Vector2f brickPos = brick->getPosition();
          sf::Color brickColor = brick->getBaseColor();

          // Apply damage to brick (1 damage per hit)
          // This prevents the same projectile from hitting the same brick
          // multiple times until it moves away from the brick (no longer
          // overlapping)
          bool wasDestroyed = brick->takeDamage(1);

          // Store first collision for bounce (only bounce once per frame)
          if (!hasCollided) {
            hasCollided = true;
            firstCollisionBounds = brickBounds;
          }

          // Create explosion effect if brick is destroyed
          if (wasDestroyed) {
            createExplosion(brickPos, brickColor);

            // Calculate and add score
            int brickMaxHealth = brick->getMaxHealth();
            int points = calculateScore(currentLevel_, brickMaxHealth);
            addScore(points);

            // Increment bricks destroyed counter
            bricksDestroyed_++;
          }

          // Allow projectile to hit multiple different bricks in the same frame
          // But prevent hitting the same brick again until it moves away
        }
      }
    }

    // Bounce projectile based on first collision (only bounce once per frame)
    // Bounce IMMEDIATELY after first collision to move projectile away from
    // brick
    if (hasCollided) {
      bounceProjectileOffBrick(projectile, firstCollisionBounds);
      // After bouncing, update projectile bounds for overlap checking
      projectileBounds = projectile->getBounds();
    }
  }
}

bool PlayingState::checkAABBCollision(const sf::FloatRect &rect1,
                                      const sf::FloatRect &rect2) const {
  // AABB collision detection (SFML 3.0: Rect uses position and size)
  return (rect1.position.x < rect2.position.x + rect2.size.x &&
          rect1.position.x + rect1.size.x > rect2.position.x &&
          rect1.position.y < rect2.position.y + rect2.size.y &&
          rect1.position.y + rect1.size.y > rect2.position.y);
}

void PlayingState::bounceProjectileOffBrick(Projectile *projectile,
                                            const sf::FloatRect &brickBounds) {
  if (!projectile) {
    return;
  }

  sf::Vector2f projectilePos = projectile->getPosition();
  sf::Vector2f projectileVel = projectile->getVelocity();

  // Calculate brick center and edges
  float brickLeft = brickBounds.position.x;
  float brickRight = brickBounds.position.x + brickBounds.size.x;
  float brickTop = brickBounds.position.y;
  float brickBottom = brickBounds.position.y + brickBounds.size.y;
  float brickCenterX = brickLeft + brickBounds.size.x / 2.0f;
  float brickCenterY = brickTop + brickBounds.size.y / 2.0f;

  // Determine which side of the brick was hit based on projectile position and
  // velocity Calculate distances to each edge
  float distToLeft = std::abs(projectilePos.x - brickLeft);
  float distToRight = std::abs(projectilePos.x - brickRight);
  float distToTop = std::abs(projectilePos.y - brickTop);
  float distToBottom = std::abs(projectilePos.y - brickBottom);

  // Find the closest edge
  float minDist = std::min({distToLeft, distToRight, distToTop, distToBottom});

  // Also consider velocity direction for more accurate bounce
  bool movingRight = projectileVel.x > 0;
  bool movingLeft = projectileVel.x < 0;
  bool movingDown = projectileVel.y > 0;
  bool movingUp = projectileVel.y < 0;

  // Calculate larger offset to ensure projectile moves completely outside brick
  // bounds Use projectile radius + collision offset to ensure clearance
  constexpr float PROJECTILE_RADIUS = 6.0f; // From Projectile.h
  float safeOffset = PROJECTILE_RADIUS + COLLISION_OFFSET;

  // Determine bounce based on closest edge and velocity direction
  // Use velocity direction as primary indicator, distance as secondary
  if (minDist == distToTop && (movingUp || projectilePos.y < brickCenterY)) {
    // Hit top edge, bounce down (reflect Y velocity downward)
    projectile->setVelocity(
        sf::Vector2f(projectileVel.x, std::abs(projectileVel.y)));
    projectile->setPosition(
        sf::Vector2f(projectilePos.x, brickTop - safeOffset));
  } else if (minDist == distToBottom &&
             (movingDown || projectilePos.y > brickCenterY)) {
    // Hit bottom edge, bounce up (reflect Y velocity upward)
    projectile->setVelocity(
        sf::Vector2f(projectileVel.x, -std::abs(projectileVel.y)));
    projectile->setPosition(
        sf::Vector2f(projectilePos.x, brickBottom + safeOffset));
  } else if (minDist == distToLeft &&
             (movingLeft || projectilePos.x < brickCenterX)) {
    // Hit left edge, bounce right (reflect X velocity rightward)
    projectile->setVelocity(
        sf::Vector2f(std::abs(projectileVel.x), projectileVel.y));
    projectile->setPosition(
        sf::Vector2f(brickLeft - safeOffset, projectilePos.y));
  } else if (minDist == distToRight &&
             (movingRight || projectilePos.x > brickCenterX)) {
    // Hit right edge, bounce left (reflect X velocity leftward)
    projectile->setVelocity(
        sf::Vector2f(-std::abs(projectileVel.x), projectileVel.y));
    projectile->setPosition(
        sf::Vector2f(brickRight + safeOffset, projectilePos.y));
  } else {
    // Fallback: determine based on velocity direction only
    if (std::abs(projectileVel.x) > std::abs(projectileVel.y)) {
      // Horizontal collision (reflect X)
      if (movingRight) {
        projectile->setVelocity(
            sf::Vector2f(-std::abs(projectileVel.x), projectileVel.y));
        projectile->setPosition(
            sf::Vector2f(brickLeft - safeOffset, projectilePos.y));
      } else if (movingLeft) {
        projectile->setVelocity(
            sf::Vector2f(std::abs(projectileVel.x), projectileVel.y));
        projectile->setPosition(
            sf::Vector2f(brickRight + safeOffset, projectilePos.y));
      }
    } else {
      // Vertical collision (reflect Y)
      if (movingDown) {
        projectile->setVelocity(
            sf::Vector2f(projectileVel.x, -std::abs(projectileVel.y)));
        projectile->setPosition(
            sf::Vector2f(projectilePos.x, brickTop - safeOffset));
      } else if (movingUp) {
        projectile->setVelocity(
            sf::Vector2f(projectileVel.x, std::abs(projectileVel.y)));
        projectile->setPosition(
            sf::Vector2f(projectilePos.x, brickBottom + safeOffset));
      }
    }
  }
}

void PlayingState::createExplosion(const sf::Vector2f &position,
                                   const sf::Color &color) {
  // Create 10-16 particles with color variation (medium complexity)
  int particleCount = 10 + (std::rand() % 7); // 10-16 particles

  for (int i = 0; i < particleCount; ++i) {
    ExplosionParticle particle;

    // Random direction and speed
    float angle = static_cast<float>(std::rand()) /
                  static_cast<float>(RAND_MAX) * 2.0f *
                  static_cast<float>(M_PI);
    float speed = 100.0f + static_cast<float>(std::rand()) /
                               static_cast<float>(RAND_MAX) *
                               100.0f; // 100-200 px/s

    particle.position = position;
    particle.velocity =
        sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);

    // Color variation: slight randomization of RGB values
    int colorVariation = 30; // +/- 30 RGB units
    int r = std::max(0, std::min(255, static_cast<int>(color.r) +
                                          (std::rand() % (colorVariation * 2)) -
                                          colorVariation));
    int g = std::max(0, std::min(255, static_cast<int>(color.g) +
                                          (std::rand() % (colorVariation * 2)) -
                                          colorVariation));
    int b = std::max(0, std::min(255, static_cast<int>(color.b) +
                                          (std::rand() % (colorVariation * 2)) -
                                          colorVariation));
    particle.color = sf::Color(r, g, b);

    // Lifetime: 0.2-0.4 seconds
    particle.maxLifetime = 0.2f + static_cast<float>(std::rand()) /
                                      static_cast<float>(RAND_MAX) * 0.2f;
    particle.lifetime = particle.maxLifetime;

    // Size: 2-4px radius
    particle.size = 2.0f + static_cast<float>(std::rand()) /
                               static_cast<float>(RAND_MAX) * 2.0f;

    explosionParticles_.push_back(particle);
  }
}

void PlayingState::updateExplosionParticles(float deltaTime) {
  // Update all particles
  for (auto &particle : explosionParticles_) {
    // Update position
    particle.position += particle.velocity * deltaTime;

    // Update lifetime
    particle.lifetime -= deltaTime;
  }

  // Remove expired particles
  explosionParticles_.erase(
      std::remove_if(explosionParticles_.begin(), explosionParticles_.end(),
                     [](const ExplosionParticle &particle) {
                       return particle.lifetime <= 0.0f;
                     }),
      explosionParticles_.end());
}

void PlayingState::renderExplosionParticles(sf::RenderWindow &window) const {
  for (const auto &particle : explosionParticles_) {
    // Calculate alpha based on lifetime
    float alphaFactor = particle.lifetime / particle.maxLifetime;
    unsigned char alpha = static_cast<unsigned char>(255 * alphaFactor);

    // Create particle shape
    sf::CircleShape particleShape(particle.size);
    particleShape.setOrigin(sf::Vector2f(particle.size, particle.size));
    particleShape.setPosition(particle.position);
    particleShape.setFillColor(
        sf::Color(particle.color.r, particle.color.g, particle.color.b, alpha));
    particleShape.setOutlineThickness(0.0f);

    window.draw(particleShape);
  }
}

int PlayingState::calculateScore(int level, int brickMaxHealth) const {
  // Calculate level multiplier (linear scaling)
  float levelMultiplier =
      LEVEL_MULTIPLIER_BASE + (level - 1) * LEVEL_MULTIPLIER_STEP;

  // Calculate health multiplier (linear scaling)
  float healthMultiplier =
      HEALTH_MULTIPLIER_BASE + (brickMaxHealth - 1) * HEALTH_MULTIPLIER_STEP;

  // Calculate final score
  float finalScore = static_cast<float>(BASE_SCORE_PER_BRICK) *
                     levelMultiplier * healthMultiplier;

  return static_cast<int>(finalScore);
}

void PlayingState::addScore(int points) {
  score_ += points;

  if (sharedMemory_) {
    sharedMemory_->lock();
    if (sharedMemory_->data()) {
      sharedMemory_->data()->currentScore = score_;
    }
    sharedMemory_->unlock();
  }
}

void PlayingState::updateScoreDisplay(float deltaTime) {
  // Animate score counting up
  if (displayedScore_ < score_) {
    int scoreDifference = score_ - displayedScore_;
    int increment = static_cast<int>(SCORE_ANIMATION_SPEED * deltaTime);

    if (increment >= scoreDifference) {
      displayedScore_ = score_;
    } else {
      displayedScore_ += increment;
    }

    // Update score text
    std::stringstream ss;
    ss << "Score: " << displayedScore_;
    scoreText_.setString(ss.str());
  } else if (displayedScore_ > score_) {
    // Safety check: if displayed score is somehow higher than actual score,
    // sync it
    displayedScore_ = score_;
    std::stringstream ss;
    ss << "Score: " << displayedScore_;
    scoreText_.setString(ss.str());
  }
}

void PlayingState::initializeScoreDisplay() {
  // Set score text position (top-left corner)
  scoreText_.setPosition(sf::Vector2f(HUD_LEFT_MARGIN, HUD_TOP_MARGIN));

  // Initialize displayed score
  displayedScore_ = score_;

  // Set initial score text
  std::stringstream ss;
  ss << "Score: " << displayedScore_;
  scoreText_.setString(ss.str());
}

void PlayingState::initializeLevelDisplay() {
  // Set level text position (below score, top-left)
  levelText_.setPosition(
      sf::Vector2f(HUD_LEFT_MARGIN, HUD_TOP_MARGIN + HUD_TEXT_SPACING));

  // Set initial level text
  std::stringstream ss;
  ss << "Level: " << currentLevel_;
  levelText_.setString(ss.str());
}

void PlayingState::initializeHighScoreDisplay() {
  // Set high score text position (top-right corner)
  // Position will be updated in onEnter() when window size is available
  highScoreText_.setPosition(sf::Vector2f(0.0f, HUD_TOP_MARGIN));

  // Set initial high score text (only if high score > 0)
  if (highScore_ > 0) {
    std::stringstream ss;
    ss << "High: " << highScore_;
    highScoreText_.setString(ss.str());
  } else {
    highScoreText_.setString("");
  }
}

void PlayingState::updateHighScoreDisplay() {
  // Update high score text position (recalculate in case window was resized)
  float windowWidth = static_cast<float>(game_->getWindowWidth());
  sf::FloatRect textBounds = highScoreText_.getLocalBounds();
  highScoreText_.setPosition(sf::Vector2f(
      windowWidth - textBounds.size.x - HUD_LEFT_MARGIN, HUD_TOP_MARGIN));

  // Update high score text (only if high score > 0)
  if (highScore_ > 0) {
    std::stringstream ss;
    ss << "High: " << highScore_;
    highScoreText_.setString(ss.str());

    // Recalculate position after updating text
    textBounds = highScoreText_.getLocalBounds();
    highScoreText_.setPosition(sf::Vector2f(
        windowWidth - textBounds.size.x - HUD_LEFT_MARGIN, HUD_TOP_MARGIN));
  } else {
    highScoreText_.setString("");
  }
}

void PlayingState::updateLevelDisplay() {
  // Update level text
  std::stringstream ss;
  ss << "Level: " << currentLevel_;
  levelText_.setString(ss.str());
}

void PlayingState::updateHUDAnimations(float deltaTime) {
  // Update animation time accumulator
  hudAnimationTime_ += deltaTime;

  // Update glow intensities using sine wave for smooth pulsing
  // Score glow: continuous pulse
  float scorePulse = std::sin(HUD_GLOW_PULSE_SPEED * hudAnimationTime_ * 2.0f *
                              static_cast<float>(M_PI));
  scoreGlowIntensity_ = HUD_GLOW_INTENSITY_MIN +
                        (HUD_GLOW_INTENSITY_MAX - HUD_GLOW_INTENSITY_MIN) *
                            (0.5f + 0.5f * scorePulse);

  // Level glow: continuous pulse (slightly offset for variation)
  float levelPulse = std::sin(HUD_GLOW_PULSE_SPEED * hudAnimationTime_ * 2.0f *
                                  static_cast<float>(M_PI) +
                              static_cast<float>(M_PI) * 0.3f);
  levelGlowIntensity_ = HUD_GLOW_INTENSITY_MIN +
                        (HUD_GLOW_INTENSITY_MAX - HUD_GLOW_INTENSITY_MIN) *
                            (0.5f + 0.5f * levelPulse);

  // High score glow: continuous pulse (slightly offset for variation)
  float highScorePulse = std::sin(HUD_GLOW_PULSE_SPEED * hudAnimationTime_ *
                                      2.0f * static_cast<float>(M_PI) +
                                  static_cast<float>(M_PI) * 0.6f);
  highScoreGlowIntensity_ = HUD_GLOW_INTENSITY_MIN +
                            (HUD_GLOW_INTENSITY_MAX - HUD_GLOW_INTENSITY_MIN) *
                                (0.5f + 0.5f * highScorePulse);

  // Update level change flash (fade out)
  if (levelChangeFlash_ > 0.0f) {
    levelChangeFlash_ -= HUD_FLASH_FADE_SPEED * deltaTime;
    levelChangeFlash_ = std::max(0.0f, levelChangeFlash_);
  }

  // Update high score flash (fade out)
  if (highScoreFlash_ > 0.0f) {
    highScoreFlash_ -= HUD_FLASH_FADE_SPEED * deltaTime;
    highScoreFlash_ = std::max(0.0f, highScoreFlash_);
  }
}

void PlayingState::renderTextWithGlow(sf::RenderWindow &window,
                                      const sf::Text &text,
                                      const sf::Color &baseColor,
                                      float glowIntensity,
                                      float flashAlpha) const {
  // Render multiple glow layers (strong cyberpunk glow)
  // Use offset-based glow (similar to AnimatedText) for better control
  for (int i = 0; i < HUD_GLOW_LAYERS; ++i) {
    float layerAlpha = static_cast<float>(HUD_GLOW_ALPHA_BASE -
                                          (i * HUD_GLOW_ALPHA_DECREMENT));
    layerAlpha = std::max(0.0f, layerAlpha);

    // Apply glow intensity and flash alpha
    float alpha = layerAlpha * glowIntensity * flashAlpha;
    alpha = std::max(0.0f, std::min(255.0f, alpha));

    // Calculate offset for this layer (increasing offset for outer layers,
    // minimal)
    float offset = static_cast<float>(i + 1) *
                   0.8f; // 0.8px, 1.6px (even smaller for minimal glow)

    // Render glow in 8 directions (up, down, left, right, and diagonals) for
    // full glow effect
    std::vector<sf::Vector2f> offsets = {
        sf::Vector2f(0.0f, -offset),    // Up
        sf::Vector2f(0.0f, offset),     // Down
        sf::Vector2f(-offset, 0.0f),    // Left
        sf::Vector2f(offset, 0.0f),     // Right
        sf::Vector2f(-offset, -offset), // Up-left
        sf::Vector2f(offset, -offset),  // Up-right
        sf::Vector2f(-offset, offset),  // Down-left
        sf::Vector2f(offset, offset)    // Down-right
    };

    // Brighten color for glow (subtle increase for smaller glow)
    unsigned char r = static_cast<unsigned char>(
        std::min(255, static_cast<int>(baseColor.r * 1.15f) + 15));
    unsigned char g = static_cast<unsigned char>(
        std::min(255, static_cast<int>(baseColor.g * 1.15f) + 15));
    unsigned char b = static_cast<unsigned char>(
        std::min(255, static_cast<int>(baseColor.b * 1.15f) + 15));

    sf::Color glowColor(r, g, b, static_cast<unsigned char>(alpha));

    // Render glow text at each offset position
    for (const auto &offsetVec : offsets) {
      sf::Text glowText = text;
      glowText.setPosition(text.getPosition() + offsetVec);
      glowText.setFillColor(glowColor);
      glowText.setOutlineColor(glowColor);
      glowText.setOutlineThickness(0.5f);
      window.draw(glowText);
    }

    // Also render center glow (slightly larger character size for inner glow)
    if (i < 2) { // Only for inner layers
      sf::Text centerGlow = text;
      unsigned int originalSize = text.getCharacterSize();
      centerGlow.setCharacterSize(static_cast<unsigned int>(
          originalSize * (1.0f + HUD_GLOW_SCALE_STEP * (i + 1))));

      // Adjust position to keep centered
      sf::FloatRect originalBounds = text.getLocalBounds();
      sf::FloatRect scaledBounds = centerGlow.getLocalBounds();
      sf::Vector2f centerOffset(
          (originalBounds.size.x - scaledBounds.size.x) / 2.0f,
          (originalBounds.size.y - scaledBounds.size.y) / 2.0f);
      centerGlow.setPosition(text.getPosition() + centerOffset);
      centerGlow.setFillColor(glowColor);
      centerGlow.setOutlineColor(glowColor);
      centerGlow.setOutlineThickness(0.5f);
      window.draw(centerGlow);
    }
  }

  // Render main text on top (with flash effect if applicable)
  sf::Text mainText = text;
  sf::Color textColor = baseColor;

  // Apply flash effect (brighten when flashing)
  if (flashAlpha > 1.0f) {
    float flashBrightness = 1.0f + (flashAlpha - 1.0f) * 0.4f;
    textColor.r = static_cast<unsigned char>(
        std::min(255, static_cast<int>(baseColor.r * flashBrightness)));
    textColor.g = static_cast<unsigned char>(
        std::min(255, static_cast<int>(baseColor.g * flashBrightness)));
    textColor.b = static_cast<unsigned char>(
        std::min(255, static_cast<int>(baseColor.b * flashBrightness)));
  }

  mainText.setFillColor(textColor);
  mainText.setOutlineColor(textColor);
  mainText.setOutlineThickness(1.0f);
  window.draw(mainText);
}

void PlayingState::triggerLevelChangeFlash() { levelChangeFlash_ = 1.0f; }

void PlayingState::triggerHighScoreFlash() { highScoreFlash_ = 1.0f; }

void PlayingState::loadHighScore() {
  std::ifstream file(HIGH_SCORE_FILE);
  if (file.is_open()) {
    file >> highScore_;
    file.close();
  } else {
    // File doesn't exist, set high score to 0
    highScore_ = 0;
  }
}

void PlayingState::saveHighScore() {
  std::ofstream file(HIGH_SCORE_FILE);
  if (file.is_open()) {
    file << highScore_;
    file.close();
  }
}
