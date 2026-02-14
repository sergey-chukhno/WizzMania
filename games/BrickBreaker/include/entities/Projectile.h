#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>
#include <algorithm>

/**
 * @brief Represents an energy projectile shot from the cannon.
 * 
 * Projectiles move with constant velocity and bounce off walls and the cannon.
 * They are destroyed when leaving the screen at the bottom.
 */
class Projectile {
public:
    /**
     * @brief Constructs an inactive projectile.
     */
    Projectile();

    /**
     * @brief Activates the projectile with the given position and velocity.
     * @param position Initial position
     * @param velocity Initial velocity (normalized direction * speed)
     */
    void activate(const sf::Vector2f& position, const sf::Vector2f& velocity);

    /**
     * @brief Deactivates the projectile.
     */
    void deactivate();

    /**
     * @brief Updates the projectile's position and handles collisions.
     * @param deltaTime Time since last update
     * @param windowSize Window size for boundary checking
     * @param cannonBounds Cannon bounding rectangle for collision
     */
    void update(float deltaTime, const sf::Vector2u& windowSize, const sf::FloatRect& cannonBounds);

    /**
     * @brief Renders the projectile with glow and trail effects.
     * @param window Render window
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Gets the projectile's position.
     * @return Position vector
     */
    sf::Vector2f getPosition() const;

    /**
     * @brief Gets the projectile's velocity.
     * @return Velocity vector
     */
    sf::Vector2f getVelocity() const;

    /**
     * @brief Gets the projectile's bounding rectangle.
     * @return Bounding rectangle
     */
    sf::FloatRect getBounds() const;

    /**
     * @brief Checks if the projectile is active.
     * @return True if active, false otherwise
     */
    bool isActive() const;

    /**
     * @brief Sets the projectile's velocity (for bouncing).
     * @param velocity New velocity vector
     */
    void setVelocity(const sf::Vector2f& velocity);

    /**
     * @brief Sets the projectile's position (for collision response).
     * @param position New position vector
     */
    void setPosition(const sf::Vector2f& position);

private:
    // Position and movement
    sf::Vector2f position_;
    sf::Vector2f velocity_;

    // State
    bool isActive_;

    // Visual
    sf::CircleShape shape_;

    // Trail system
    struct TrailPoint {
        sf::Vector2f position;
        float alpha;
        float age;
    };
    std::deque<TrailPoint> trailPoints_;
    static constexpr size_t MAX_TRAIL_POINTS = 8;
    static constexpr float TRAIL_UPDATE_INTERVAL = 0.02f; // Update trail every 20ms
    float trailUpdateTimer_;

    // Constants
    static constexpr float PROJECTILE_RADIUS = 6.0f;
    static constexpr float PROJECTILE_SPEED = 600.0f; // pixels per second
    static constexpr sf::Color PROJECTILE_COLOR = sf::Color(255, 136, 0); // Bright orange/flame (#FF8800)
    static constexpr int GLOW_LAYERS = 3;
    static constexpr float TRAIL_LIFETIME = 0.16f; // Trail points live for 160ms
    static constexpr float TRAIL_FADE_SPEED = 1.0f / TRAIL_LIFETIME;

    // Collision offset to prevent sticking
    static constexpr float COLLISION_OFFSET = 1.0f;

    // Helper methods
    void updateTrail(float deltaTime);
    void checkWallCollisions(const sf::Vector2u& windowSize);
    void checkCannonCollision(const sf::FloatRect& cannonBounds);
    void renderGlow(sf::RenderWindow& window) const;
    void renderTrail(sf::RenderWindow& window) const;
};

/**
 * @brief Object pool for managing projectiles efficiently.
 */
class ProjectilePool {
public:
    /**
     * @brief Constructs a projectile pool with the given size.
     * @param poolSize Maximum number of projectiles in the pool
     */
    explicit ProjectilePool(size_t poolSize = 100);

    /**
     * @brief Gets an inactive projectile from the pool.
     * @param position Initial position
     * @param velocity Initial velocity
     * @return Pointer to activated projectile, or nullptr if pool is full
     */
    Projectile* acquire(const sf::Vector2f& position, const sf::Vector2f& velocity);

    /**
     * @brief Updates all active projectiles.
     * @param deltaTime Time since last update
     * @param windowSize Window size for boundary checking
     * @param cannonBounds Cannon bounding rectangle for collision
     */
    void updateAll(float deltaTime, const sf::Vector2u& windowSize, const sf::FloatRect& cannonBounds);

    /**
     * @brief Renders all active projectiles.
     * @param window Render window
     */
    void renderAll(sf::RenderWindow& window) const;

    /**
     * @brief Gets the number of active projectiles.
     * @return Number of active projectiles
     */
    size_t getActiveCount() const;

    /**
     * @brief Gets all active projectiles (for collision detection).
     * @return Vector of active projectile pointers
     */
    std::vector<Projectile*> getActiveProjectiles();

    /**
     * @brief Gets all active projectiles (const version).
     * @return Vector of const active projectile pointers
     */
    std::vector<const Projectile*> getActiveProjectiles() const;

    /**
     * @brief Clears all projectiles (deactivates all).
     */
    void clear();

private:
    std::vector<Projectile> pool_;
    size_t activeCount_;
};

