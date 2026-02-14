#ifndef BRICK_H
#define BRICK_H

#include <SFML/Graphics.hpp>

/**
 * @brief Represents an individual brick within a block.
 * 
 * Bricks have individual health, position, and visual representation.
 * They can be destroyed independently and contribute to block destruction.
 */
class Brick {
public:
    /**
     * @brief Constructs a brick at the specified position.
     * @param position Brick position in world coordinates
     * @param size Brick size (width, height)
     * @param maxHealth Maximum health (durability)
     * @param baseColor Base color from block's palette
     * @param distanceFactor Distance factor from block center (0.0 = edge, 1.0 = center) for gradient
     */
    Brick(const sf::Vector2f& position, const sf::Vector2f& size, int maxHealth, const sf::Color& baseColor, float distanceFactor = 1.0f);

    /**
     * @brief Destructor.
     */
    ~Brick() = default;

    /**
     * @brief Updates the brick state.
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the brick with glow and health indicator.
     * @param window Reference to the render window
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Applies damage to the brick.
     * @param amount Damage amount (default: 1)
     * @return True if brick is destroyed, false otherwise
     */
    bool takeDamage(int amount = 1);

    /**
     * @brief Checks if the brick is destroyed.
     * @return True if destroyed, false otherwise
     */
    bool isDestroyed() const;

    /**
     * @brief Gets the bounding rectangle of the brick.
     * @return Bounding rectangle
     */
    sf::FloatRect getBounds() const;

    /**
     * @brief Gets the position of the brick.
     * @return Position vector
     */
    sf::Vector2f getPosition() const;

    /**
     * @brief Gets the current health of the brick.
     * @return Current health
     */
    int getHealth() const;

    /**
     * @brief Gets the maximum health of the brick.
     * @return Maximum health
     */
    int getMaxHealth() const;

    /**
     * @brief Sets the position of the brick.
     * @param position New position
     */
    void setPosition(const sf::Vector2f& position);

    /**
     * @brief Gets the base color of the brick (for explosion effects).
     * @return Base color
     */
    sf::Color getBaseColor() const;

private:
    // Position and size
    sf::Vector2f position_;
    sf::Vector2f size_;

    // Health system
    int health_;
    int maxHealth_;
    bool isDestroyed_;
    float distanceFactor_;  // Distance factor from block center (0.0 = edge, 1.0 = center) for gradient

    // Visual
    sf::RectangleShape shape_;
    sf::Color baseColor_;
    sf::Color currentColor_;
    sf::Text healthText_;

    // Animation state (asteroid-like effects)
    float rotationAngle_;           // Rotation angle for asteroid effect
    float rotationSpeed_;           // Rotation speed (degrees per second)
    float pulseAlpha_;              // Pulse alpha for glow animation (0.6-1.0)
    float glowIntensity_;           // Glow intensity for animation
    float animationTime_;           // Time accumulator for animations

    // Visual constants
    static constexpr float OUTLINE_THICKNESS = 2.0f;    // Outline thickness (thicker for asteroid look)

    // Glow effect constants (enhanced for better visibility)
    static constexpr int GLOW_LAYERS = 6;               // Number of glow layers (more layers for stronger glow)
    static constexpr float GLOW_SCALE_STEP = 0.15f;     // Scale step per layer (larger steps for more visible glow)
    static constexpr unsigned char GLOW_ALPHA_BASE = 120; // Base alpha for glow (much more visible, 120/255 = ~47%)
    static constexpr unsigned char GLOW_ALPHA_DECREMENT = 18; // Alpha decrement per layer (more gradual fade)
    
    // Animation constants
    static constexpr float ROTATION_SPEED_MIN = 10.0f;  // Minimum rotation speed (degrees per second)
    static constexpr float ROTATION_SPEED_MAX = 30.0f;  // Maximum rotation speed (degrees per second)
    static constexpr float PULSE_SPEED = 2.0f;          // Pulse speed (full cycle per second)
    static constexpr float PULSE_ALPHA_MIN = 0.6f;      // Minimum pulse alpha
    static constexpr float PULSE_ALPHA_MAX = 1.0f;      // Maximum pulse alpha
    static constexpr float GLOW_INTENSITY_MIN = 0.7f;   // Minimum glow intensity
    static constexpr float GLOW_INTENSITY_MAX = 1.0f;   // Maximum glow intensity

    // Health text constants
    static constexpr unsigned int HEALTH_TEXT_SIZE = 16; // Font size for health text (increased for larger bricks)
    static constexpr sf::Color HEALTH_TEXT_COLOR = sf::Color(255, 255, 255); // White

    /**
     * @brief Updates the brick color based on distance factor (gradient: darker = center, lighter = edges).
     */
    void updateColor();

    /**
     * @brief Updates the health text display.
     */
    void updateHealthText();

    /**
     * @brief Updates animation state (rotation, pulse, glow).
     * @param deltaTime Time elapsed since last frame
     */
    void updateAnimations(float deltaTime);

    /**
     * @brief Renders glow effect for the brick.
     * @param window Render window
     * @param shape Rectangle shape to add glow to
     * @param baseColor Base color for glow
     */
    void renderGlow(sf::RenderWindow& window, const sf::RectangleShape& shape, const sf::Color& baseColor) const;

    /**
     * @brief Adjusts color brightness (darker = center, lighter = edges).
     * @param color Base color
     * @param distanceFactor Distance factor from block center (0.0 = edge, 1.0 = center)
     * @return Adjusted color (darker for center, lighter for edges)
     */
    static sf::Color adjustBrightness(const sf::Color& color, float distanceFactor);
};

#endif // BRICK_H

