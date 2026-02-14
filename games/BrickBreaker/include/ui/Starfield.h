#ifndef STARFIELD_H
#define STARFIELD_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

/**
 * @brief Animated starfield background component.
 * 
 * Starfield provides an animated background with:
 * - Configurable number of stars
 * - Twinkling animation (alpha pulsing)
 * - Slow drift movement
 * - Pink and cyan colored stars
 */
class Starfield
{
public:
    /**
     * @brief Star structure for individual star properties.
     */
    struct Star
    {
        sf::Vector2f position;      // Star position
        sf::Color color;            // Star color (pink or cyan)
        float radius;               // Star radius (1-3px)
        float alpha;                // Current alpha (0.3 to 1.0)
        float twinkleSpeed;         // Alpha change per second
        float twinkleDirection;     // 1.0 (increasing) or -1.0 (decreasing)
        float minAlpha;             // Minimum alpha (0.3)
        float maxAlpha;             // Maximum alpha (1.0)
        sf::Vector2f velocity;      // Drift velocity (pixels per second)
    };

    /**
     * @brief Constructs a Starfield.
     * @param starCount Number of stars to create (default: 200)
     * @param windowSize Window size for star positioning
     */
    Starfield(unsigned int starCount = 200,
              const sf::Vector2u& windowSize = sf::Vector2u(1280, 720));

    /**
     * @brief Updates star animations (twinkling and drift).
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the starfield.
     * @param window Render window to draw to
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Sets the window size (for star repositioning on resize).
     * @param size New window size
     */
    void setWindowSize(const sf::Vector2u& size);

private:
    std::vector<Star> stars_;       // Vector of stars
    sf::Vector2u windowSize_;       // Window size
    mutable std::mt19937 rng_;      // Random number generator (mutable for const methods)

    // Constants
    static constexpr unsigned int DEFAULT_STAR_COUNT = 200;
    static constexpr float STAR_MIN_ALPHA = 0.3f;
    static constexpr float STAR_MAX_ALPHA = 1.0f;
    static constexpr float STAR_MIN_TWINKLE_SPEED = 0.5f;
    static constexpr float STAR_MAX_TWINKLE_SPEED = 2.0f;
    static constexpr float STAR_MIN_RADIUS = 1.0f;
    static constexpr float STAR_MAX_RADIUS = 3.0f;
    static constexpr float STAR_DRIFT_SPEED_MIN = 10.0f;  // Pixels per second
    static constexpr float STAR_DRIFT_SPEED_MAX = 30.0f;  // Pixels per second

    // Colors (from ANALYSIS.md) - using static functions since constexpr Color with parameters isn't available
    static sf::Color getPinkColor() { return sf::Color(255, 0, 110); }   // #ff006e
    static sf::Color getCyanColor() { return sf::Color(0, 217, 255); }   // #00d9ff

    /**
     * @brief Initializes stars with random properties.
     */
    void initializeStars();

    /**
     * @brief Generates a random float between min and max.
     * @param min Minimum value
     * @param max Maximum value
     * @return Random float
     */
    float randomFloat(float min, float max) const;

    /**
     * @brief Clamps a value between min and max.
     * @param value Value to clamp
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    static float clamp(float value, float min, float max);
};

#endif // STARFIELD_H

