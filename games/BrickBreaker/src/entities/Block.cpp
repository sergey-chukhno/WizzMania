#include "entities/Block.h"
#include "entities/Brick.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Block::Block(const sf::Vector2f& position, BlockShape shapeType, const sf::Color& baseColor, int level)
    : position_(position)
    , velocity_(0.0f, 0.0f)
    , shapeType_(shapeType)
    , baseColor_(baseColor)
    , level_(level)
    , isDestroyed_(false)
{
    // Initialize bricks with random arrangement
    initializeBricks();
    
    // Store relative positions of bricks for movement
    // Bricks are stored with world positions, but we'll update them relative to block center
}

void Block::update(float deltaTime)
{
    if (isDestroyed_) {
        return;
    }

    // Update position based on velocity (for descent)
    position_ += velocity_ * deltaTime;

    // Update all bricks (move them with the block)
    for (size_t i = 0; i < bricks_.size(); ++i) {
        if (bricks_[i] && !bricks_[i]->isDestroyed()) {
            // Update brick position relative to block movement
            if (i < brickRelativePositions_.size()) {
                // Update world position of brick based on new block position
                sf::Vector2f newWorldPosition = position_ + brickRelativePositions_[i];
                bricks_[i]->setPosition(newWorldPosition);
            }
            // Update brick state
            bricks_[i]->update(deltaTime);
        }
    }

    // Check if all bricks are destroyed
    bool allDestroyed = true;
    for (const auto& brick : bricks_) {
        if (brick && !brick->isDestroyed()) {
            allDestroyed = false;
            break;
        }
    }

    if (allDestroyed) {
        isDestroyed_ = true;
    }
}

void Block::render(sf::RenderWindow& window) const
{
    if (isDestroyed_) {
        return;
    }

    // Render all bricks
    for (const auto& brick : bricks_) {
        if (brick && !brick->isDestroyed()) {
            brick->render(window);
        }
    }
}

bool Block::isDestroyed() const
{
    return isDestroyed_;
}

sf::FloatRect Block::getBounds() const
{
    if (isDestroyed_ || bricks_.empty()) {
        return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0));
    }

    // Calculate overall bounds from all bricks
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& brick : bricks_) {
        if (brick && !brick->isDestroyed()) {
            sf::FloatRect bounds = brick->getBounds();
            minX = std::min(minX, bounds.position.x);
            minY = std::min(minY, bounds.position.y);
            maxX = std::max(maxX, bounds.position.x + bounds.size.x);
            maxY = std::max(maxY, bounds.position.y + bounds.size.y);
        }
    }

    if (minX == std::numeric_limits<float>::max()) {
        return sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(0, 0));
    }

    return sf::FloatRect(
        sf::Vector2f(minX, minY),
        sf::Vector2f(maxX - minX, maxY - minY)
    );
}

sf::Vector2f Block::getPosition() const
{
    return position_;
}

int Block::getLevel() const
{
    return level_;
}

BlockShape Block::getShapeType() const
{
    return shapeType_;
}

void Block::setPosition(const sf::Vector2f& position)
{
    position_ = position;

    // Update all brick positions based on relative positions
    for (size_t i = 0; i < bricks_.size(); ++i) {
        if (bricks_[i] && i < brickRelativePositions_.size()) {
            sf::Vector2f newWorldPosition = position_ + brickRelativePositions_[i];
            bricks_[i]->setPosition(newWorldPosition);
        }
    }
}

void Block::setVelocity(const sf::Vector2f& velocity)
{
    velocity_ = velocity;
}

sf::Vector2f Block::getVelocity() const
{
    return velocity_;
}

std::vector<Brick*> Block::getBricks()
{
    std::vector<Brick*> result;
    for (auto& brick : bricks_) {
        if (brick && !brick->isDestroyed()) {
            result.push_back(brick.get());
        }
    }
    return result;
}

std::vector<const Brick*> Block::getBricks() const
{
    std::vector<const Brick*> result;
    for (const auto& brick : bricks_) {
        if (brick && !brick->isDestroyed()) {
            result.push_back(brick.get());
        }
    }
    return result;
}

sf::Color Block::getRandomColor()
{
    // Random selection from cyberpunk color palette
    static const std::vector<sf::Color> colors = {
        COLOR_PINK,
        COLOR_CYAN,
        COLOR_PURPLE,
        COLOR_GREEN,
        COLOR_YELLOW
    };

    int randomIndex = std::rand() % colors.size();
    return colors[randomIndex];
}

BlockShape Block::getRandomShape(int level)
{
    // Shape distribution based on level
    // Level 1: Mostly squares (80%), some rectangles (20%)
    // Level 2: Squares (60%), rectangles (40%)
    // Level 3: Squares (40%), rectangles (40%), hexagons (20%)
    // Level 4: Squares (30%), rectangles (40%), hexagons (30%)
    // Level 5+: Squares (20%), rectangles (30%), hexagons (50%)

    int randomValue = std::rand() % 100;

    if (level == 1) {
        // Level 1: 80% squares, 20% rectangles
        if (randomValue < 80) {
            return BlockShape::Square;
        } else {
            return BlockShape::Rectangle;
        }
    } else if (level == 2) {
        // Level 2: 60% squares, 40% rectangles
        if (randomValue < 60) {
            return BlockShape::Square;
        } else {
            return BlockShape::Rectangle;
        }
    } else if (level == 3) {
        // Level 3: 40% squares, 40% rectangles, 20% hexagons
        if (randomValue < 40) {
            return BlockShape::Square;
        } else if (randomValue < 80) {
            return BlockShape::Rectangle;
        } else {
            return BlockShape::Hexagon;
        }
    } else if (level == 4) {
        // Level 4: 30% squares, 40% rectangles, 30% hexagons
        if (randomValue < 30) {
            return BlockShape::Square;
        } else if (randomValue < 70) {
            return BlockShape::Rectangle;
        } else {
            return BlockShape::Hexagon;
        }
    } else {
        // Level 5+: 20% squares, 30% rectangles, 50% hexagons
        if (randomValue < 20) {
            return BlockShape::Square;
        } else if (randomValue < 50) {
            return BlockShape::Rectangle;
        } else {
            return BlockShape::Hexagon;
        }
    }
}

void Block::initializeBricks()
{
    bricks_.clear();
    brickRelativePositions_.clear();

    // Calculate number of bricks based on shape and level
    int brickCount = calculateBrickCount(shapeType_, level_);

    // Get max distance for health calculation
    float maxDistance = getMaxDistance();

    // Store existing brick positions for overlap checking
    std::vector<sf::Vector2f> existingPositions;

    // Create bricks with random arrangement
    for (int i = 0; i < brickCount; ++i) {
        sf::Vector2f brickPosition;
        bool placed = false;
        int attempts = 0;

        // Try to place brick (up to MAX_PLACEMENT_ATTEMPTS)
        while (!placed && attempts < MAX_PLACEMENT_ATTEMPTS) {
            // Generate random position within block bounds
            brickPosition = generateRandomPosition();

            // Check if position is within bounds
            if (!isWithinBounds(brickPosition)) {
                attempts++;
                continue;
            }

            // Check if position overlaps with existing bricks
            if (!overlapsWithExisting(brickPosition, existingPositions)) {
                placed = true;
                existingPositions.push_back(brickPosition);
            } else {
                attempts++;
            }
        }

        // If brick was placed, create it
        if (placed) {
            // Calculate distance from block center for health and gradient
            float distance = std::sqrt(brickPosition.x * brickPosition.x + brickPosition.y * brickPosition.y);
            
            // Calculate distance factor (0.0 = edge, 1.0 = center) for gradient
            float distanceFactor = 1.0f - (distance / maxDistance);
            distanceFactor = std::max(0.0f, std::min(1.0f, distanceFactor)); // Clamp to [0, 1]
            
            // Calculate brick health based on distance from center (pattern: center stronger, edges weaker)
            int brickHealth = calculateBrickHealth(brickPosition, maxDistance, level_);

            // Store relative position (relative to block center)
            brickRelativePositions_.push_back(brickPosition);

            // Calculate world position (relative to block center)
            sf::Vector2f worldPosition = position_ + brickPosition;

            // Create brick with distance factor for gradient
            bricks_.push_back(std::make_unique<Brick>(
                worldPosition,
                sf::Vector2f(BRICK_WIDTH, BRICK_HEIGHT),
                brickHealth,
                baseColor_,
                distanceFactor  // Pass distance factor for gradient
            ));
        }
        // If brick couldn't be placed, skip it (don't create)
    }
}

int Block::calculateBrickCount(BlockShape shapeType, int level)
{
    // Brick count varies by shape and level
    // Level 1: 4-6 bricks
    // Level 2: 6-9 bricks
    // Level 3: 9-12 bricks
    // Level 4: 12-16 bricks
    // Level 5+: 16-20+ bricks

    int minCount, maxCount;

    // Base count range by level
    if (level == 1) {
        minCount = 4;
        maxCount = 6;
    } else if (level == 2) {
        minCount = 6;
        maxCount = 9;
    } else if (level == 3) {
        minCount = 9;
        maxCount = 12;
    } else if (level == 4) {
        minCount = 12;
        maxCount = 16;
    } else {
        minCount = 16;
        maxCount = 20;
    }

    // Adjust by shape type
    if (shapeType == BlockShape::Square) {
        // Square: Slightly more bricks (grid-like)
        maxCount += 2;
    } else if (shapeType == BlockShape::Rectangle) {
        // Rectangle: Medium bricks
        // No adjustment
    } else if (shapeType == BlockShape::Hexagon) {
        // Hexagon: Varied bricks (hexagonal arrangement)
        minCount += 1;
        maxCount += 3;
    }

    // Random count within range
    int range = maxCount - minCount + 1;
    return minCount + (std::rand() % range);
}

int Block::calculateBrickHealth(const sf::Vector2f& brickPosition, float maxDistance, int level)
{
    // Calculate distance from block center (0, 0)
    float distance = std::sqrt(brickPosition.x * brickPosition.x + brickPosition.y * brickPosition.y);

    // Calculate health factor (0.0 = edge, 1.0 = center)
    // Continuous distance-based formula
    float healthFactor = 1.0f - (distance / maxDistance);
    healthFactor = std::max(0.0f, std::min(1.0f, healthFactor)); // Clamp to [0, 1]

    // Base health range by level (increased range for better differentiation)
    int baseHealth, maxHealth;
    if (level == 1) {
        baseHealth = 1;
        maxHealth = 4;  // Increased from 3 to 4 for better differentiation
    } else if (level == 2) {
        baseHealth = 1;
        maxHealth = 5;  // Increased from 4 to 5
    } else if (level == 3) {
        baseHealth = 2;
        maxHealth = 6;  // Increased from 5 to 6
    } else if (level == 4) {
        baseHealth = 2;
        maxHealth = 7;  // Increased from 6 to 7
    } else {
        baseHealth = 3;
        maxHealth = 10;  // Increased from 8 to 10
    }

    // Calculate brick health based on health factor
    // Center bricks (healthFactor = 1.0): maxHealth
    // Edge bricks (healthFactor = 0.0): baseHealth
    // Use rounding instead of truncation for better distribution
    int healthRange = maxHealth - baseHealth;
    float healthFloat = baseHealth + (healthFactor * healthRange);
    int brickHealth = static_cast<int>(std::round(healthFloat));

    // Ensure minimum health of 1 and clamp to range
    return std::max(1, std::min(maxHealth, brickHealth));
}

sf::FloatRect Block::getBlockBounds() const
{
    // Return block bounds relative to block center (0, 0)
    if (shapeType_ == BlockShape::Square) {
        float halfSize = SQUARE_SIZE / 2.0f;
        return sf::FloatRect(
            sf::Vector2f(-halfSize, -halfSize),
            sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE)
        );
    } else if (shapeType_ == BlockShape::Rectangle) {
        float halfWidth = RECTANGLE_WIDTH / 2.0f;
        float halfHeight = RECTANGLE_HEIGHT / 2.0f;
        return sf::FloatRect(
            sf::Vector2f(-halfWidth, -halfHeight),
            sf::Vector2f(RECTANGLE_WIDTH, RECTANGLE_HEIGHT)
        );
    } else { // Hexagon
        float radius = HEXAGON_RADIUS;
        return sf::FloatRect(
            sf::Vector2f(-radius, -radius),
            sf::Vector2f(radius * 2.0f, radius * 2.0f)
        );
    }
}

float Block::getMaxDistance() const
{
    // Calculate maximum distance from center for health calculation
    sf::FloatRect bounds = getBlockBounds();
    
    // For square and rectangle, use diagonal distance
    if (shapeType_ == BlockShape::Square || shapeType_ == BlockShape::Rectangle) {
        float halfWidth = bounds.size.x / 2.0f;
        float halfHeight = bounds.size.y / 2.0f;
        return std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight);
    } else { // Hexagon
        return HEXAGON_RADIUS;
    }
}

bool Block::isWithinBounds(const sf::Vector2f& position) const
{
    // Check if position is within block bounds (accounting for brick size)
    sf::FloatRect bounds = getBlockBounds();
    float halfBrickWidth = BRICK_WIDTH / 2.0f;
    float halfBrickHeight = BRICK_HEIGHT / 2.0f;

    if (shapeType_ == BlockShape::Square || shapeType_ == BlockShape::Rectangle) {
        // Rectangle bounds check
        return (position.x - halfBrickWidth >= bounds.position.x &&
                position.x + halfBrickWidth <= bounds.position.x + bounds.size.x &&
                position.y - halfBrickHeight >= bounds.position.y &&
                position.y + halfBrickHeight <= bounds.position.y + bounds.size.y);
    } else { // Hexagon
        // Circle bounds check (distance from center)
        // Use the larger dimension (width) for radius check
        float distance = std::sqrt(position.x * position.x + position.y * position.y);
        float maxRadius = HEXAGON_RADIUS - std::max(halfBrickWidth, halfBrickHeight);
        return distance <= maxRadius;
    }
}

bool Block::overlapsWithExisting(const sf::Vector2f& position, const std::vector<sf::Vector2f>& existingPositions) const
{
    // Check if position overlaps with any existing brick position
    // Strict overlap prevention: no overlap allowed (AABB collision check)
    // Use AABB (Axis-Aligned Bounding Box) for rectangular bricks
    
    float halfWidth = BRICK_WIDTH / 2.0f;
    float halfHeight = BRICK_HEIGHT / 2.0f;
    
    // Bounds of new brick
    float newLeft = position.x - halfWidth;
    float newRight = position.x + halfWidth;
    float newTop = position.y - halfHeight;
    float newBottom = position.y + halfHeight;

    for (const auto& existingPos : existingPositions) {
        // Bounds of existing brick
        float existingLeft = existingPos.x - halfWidth;
        float existingRight = existingPos.x + halfWidth;
        float existingTop = existingPos.y - halfHeight;
        float existingBottom = existingPos.y + halfHeight;

        // AABB collision check
        if (newLeft < existingRight && newRight > existingLeft &&
            newTop < existingBottom && newBottom > existingTop) {
            return true; // Overlaps
        }
    }

    return false; // No overlap
}

sf::Vector2f Block::generateRandomPosition() const
{
    // Generate random position within block bounds
    sf::FloatRect bounds = getBlockBounds();
    float halfBrickWidth = BRICK_WIDTH / 2.0f;
    float halfBrickHeight = BRICK_HEIGHT / 2.0f;

    if (shapeType_ == BlockShape::Square || shapeType_ == BlockShape::Rectangle) {
        // Random position within rectangle bounds (accounting for brick size)
        float minX = bounds.position.x + halfBrickWidth;
        float maxX = bounds.position.x + bounds.size.x - halfBrickWidth;
        float minY = bounds.position.y + halfBrickHeight;
        float maxY = bounds.position.y + bounds.size.y - halfBrickHeight;

        float x = minX + static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (maxX - minX);
        float y = minY + static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (maxY - minY);

        return sf::Vector2f(x, y);
    } else { // Hexagon
        // Random position within circle bounds (accounting for brick size)
        // Use the larger dimension for radius check
        float maxRadius = HEXAGON_RADIUS - std::max(halfBrickWidth, halfBrickHeight);
        
        // Generate random angle and radius
        float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * static_cast<float>(M_PI);
        float radius = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * maxRadius;

        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);

        return sf::Vector2f(x, y);
    }
}

int Block::calculateMaxHealth(int level)
{
    // Legacy method: Calculate max health for a single-brick block
    // This is kept for compatibility but not used in the multi-brick system
    int baseHealth = 1;
    int healthScaling = std::max(0, (level - 1) / 2);
    int randomVariation = (std::rand() % 2);
    return std::max(1, baseHealth + healthScaling + randomVariation);
}
