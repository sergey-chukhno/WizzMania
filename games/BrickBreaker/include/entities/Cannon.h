#ifndef CANNON_H
#define CANNON_H

#include <SFML/Graphics.hpp>

/**
 * @brief Cannon class that can be rotated to adjust shooting angle.
 * 
 * The cannon is fixed at the bottom center of the screen and can be rotated
 * left/right to adjust the angle at which projectiles are shot. The cannon
 * displays a projectile counter and supports both keyboard and mouse input
 * for angle control.
 */
class Cannon
{
public:
    /**
     * @brief Constructs a Cannon at the specified position.
     * @param position Position of the cannon (typically bottom center)
     * @param projectileCount Initial number of projectiles (default: 10)
     */
    Cannon(const sf::Vector2f& position, unsigned int projectileCount = 10);

    /**
     * @brief Destructor.
     */
    ~Cannon() = default;

    /**
     * @brief Updates the cannon state.
     * @param deltaTime Time elapsed since last frame (in seconds)
     * @param window Reference to the render window (for mouse position)
     */
    void update(float deltaTime, const sf::RenderWindow& window);

    /**
     * @brief Renders the cannon.
     * @param window Reference to the render window
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Handles input events.
     * @param event SFML event
     * @param window Reference to the render window
     */
    void handleInput(const sf::Event& event, const sf::RenderWindow& window);

    /**
     * @brief Gets the bounding rectangle of the cannon.
     * @return Bounding rectangle
     */
    sf::FloatRect getBounds() const;

    /**
     * @brief Gets the position of the cannon.
     * @return Position vector
     */
    sf::Vector2f getPosition() const;

    /**
     * @brief Gets the current angle of the cannon.
     * @return Angle in degrees (0° = straight up, positive = right, negative = left)
     */
    float getAngle() const;

    /**
     * @brief Gets the normalized direction vector for shooting.
     * @return Normalized direction vector
     */
    sf::Vector2f getShootDirection() const;

    /**
     * @brief Gets the current projectile count.
     * @return Number of projectiles remaining
     */
    unsigned int getProjectileCount() const;

    /**
     * @brief Checks if the cannon can shoot.
     * @return True if projectiles are available, false otherwise
     */
    bool canShoot() const;

    /**
     * @brief Decrements the projectile count.
     * Called when a projectile is shot.
     */
    void decrementProjectileCount();

    /**
     * @brief Sets the projectile count to a specific value.
     * Used to reset projectiles at the start of a new level.
     * @param count New projectile count
     */
    void setProjectileCount(unsigned int count);

    /**
     * @brief Shoots a projectile.
     * Returns the spawn position and velocity for the projectile.
     * @param[out] spawnPosition Spawn position of the projectile
     * @param[out] velocity Initial velocity of the projectile
     * @return True if projectile was shot, false if no projectiles available
     */
    bool shoot(sf::Vector2f& spawnPosition, sf::Vector2f& velocity);

private:
    // Angle control constants
    static constexpr float DEFAULT_ANGLE = 0.0f;        // 0° (straight up)
    static constexpr float MIN_ANGLE = -45.0f;          // -45° (left)
    static constexpr float MAX_ANGLE = 45.0f;           // +45° (right)
    static constexpr float ANGLE_SPEED = 90.0f;         // 90 degrees per second
    static constexpr float MOUSE_SENSITIVITY = 1.0f;    // Mouse sensitivity factor

    // Position (fixed at bottom center)
    sf::Vector2f position_;

    // Angle control
    float angle_;              // Current angle in degrees
    float angleDirection_;     // -1.0 (rotate left), 0.0 (stop), 1.0 (rotate right)

    // Mouse control
    bool useMouseControl_;     // Enable mouse control
    
    // Animation state
    float corePulseAlpha_;     // Current pulse alpha for energy core (0.5 to 1.0)
    float corePulseDirection_; // Pulse direction (-1.0 = decreasing, 1.0 = increasing)
    float glowIntensity_;      // Glow intensity for base and barrel (0.5 to 1.0)
    
    // Shooting effects state
    float shootingEffectTimer_;    // Timer for shooting visual effects
    bool isShooting_;              // Whether shooting effect is active
    float barrelRedPulseAlpha_;    // Red pulse alpha for barrel (0.0 to 1.0)
    float lightningGlowAlpha_;     // White lightning glow alpha (0.0 to 1.0)
    float pulseTime_;              // Time accumulator for pulse animation
    float flameAnimationTime_;     // Time accumulator for flame animation

    // Visual - Base components (stationary)
    sf::RectangleShape baseBody_;      // Main base body
    sf::RectangleShape baseLeftPanel_; // Left side panel
    sf::RectangleShape baseRightPanel_; // Right side panel
    sf::CircleShape baseCore_;         // Central core/energy core
    sf::RectangleShape baseFrontPanel_; // Front panel
    
    // Visual - Barrel components (rotatable)
    sf::RectangleShape barrelMain_;    // Main barrel segment
    sf::RectangleShape barrelMuzzle_;  // Muzzle opening
    sf::CircleShape barrelRing1_;      // Energy ring 1
    sf::CircleShape barrelRing2_;      // Energy ring 2
    sf::RectangleShape barrelFin1_;    // Top fin
    sf::RectangleShape barrelFin2_;    // Bottom fin
    
    sf::Text counterText_;             // Projectile counter text

    // Projectile management
    unsigned int projectileCount_;

    // Visual constants - Base
    static constexpr float BASE_WIDTH = 50.0f;          // Base width
    static constexpr float BASE_HEIGHT = 35.0f;         // Base height
    static constexpr float BASE_PANEL_WIDTH = 12.0f;    // Side panel width
    static constexpr float BASE_PANEL_HEIGHT = 25.0f;   // Side panel height
    static constexpr float BASE_CORE_RADIUS = 8.0f;     // Core radius
    static constexpr float BASE_FRONT_WIDTH = 20.0f;    // Front panel width
    static constexpr float BASE_FRONT_HEIGHT = 15.0f;   // Front panel height
    
    // Visual constants - Barrel
    static constexpr float BARREL_LENGTH = 70.0f;       // Barrel length
    static constexpr float BARREL_WIDTH = 18.0f;        // Barrel main width
    static constexpr float BARREL_MUZZLE_WIDTH = 22.0f; // Muzzle width (wider)
    static constexpr float BARREL_MUZZLE_LENGTH = 8.0f; // Muzzle length
    static constexpr float BARREL_RING_RADIUS = 10.0f;  // Energy ring radius
    static constexpr float BARREL_FIN_WIDTH = 4.0f;     // Fin width
    static constexpr float BARREL_FIN_LENGTH = 15.0f;   // Fin length
    
    static constexpr float COUNTER_OFFSET_Y = 25.0f;    // Counter Y offset below cannon base
    
    // Animation constants
    static constexpr float CORE_PULSE_SPEED = 2.0f;     // Pulse speed (alpha change per second)
    static constexpr float CORE_PULSE_MIN_ALPHA = 0.5f; // Minimum pulse alpha
    static constexpr float CORE_PULSE_MAX_ALPHA = 1.0f; // Maximum pulse alpha
    static constexpr float GLOW_INTENSITY_MIN = 0.6f;   // Minimum glow intensity
    static constexpr float GLOW_INTENSITY_MAX = 1.0f;   // Maximum glow intensity
    static constexpr float GLOW_RADIUS = 8.0f;          // Glow radius for layers
    static constexpr int GLOW_LAYERS = 3;               // Number of glow layers
    
    // Shooting effects constants
    static constexpr float SHOOTING_EFFECT_DURATION = 0.30f; // Duration of shooting effect (300ms, doubled from 150ms)
    static constexpr float BARREL_RED_PULSE_SPEED = 8.0f;    // Red pulse speed for barrel
    static constexpr float LIGHTNING_GLOW_SPEED = 12.0f;     // Lightning glow fade speed
    static constexpr float FLAME_ANIMATION_SPEED = 20.0f;    // Flame animation speed
    static constexpr int FLAME_LAYERS = 5;                   // Number of flame layers for depth
    static constexpr float FLAME_BASE_SIZE = 12.0f;          // Base flame size
    static constexpr float FLAME_MAX_SIZE = 20.0f;           // Maximum flame size
    static constexpr float CONTOUR_FLAME_SIZE = 4.0f;        // Size of contour flame particles
    static constexpr float CONTOUR_FLAME_SPACING = 8.0f;     // Spacing between contour flames
    static constexpr int CONTOUR_FLAME_POINTS = 3;           // Number of flame points per contour segment

    // Colors
    static constexpr sf::Color CANNON_PRIMARY = sf::Color(0, 217, 255);    // Cyan #00d9ff
    static constexpr sf::Color CANNON_SECONDARY = sf::Color(0, 180, 220);  // Darker cyan
    static constexpr sf::Color CANNON_ACCENT = sf::Color(100, 255, 255);   // Bright cyan
    static constexpr sf::Color CANNON_OUTLINE = sf::Color(0, 217, 255);    // Cyan outline
    static constexpr sf::Color CANNON_CORE = sf::Color(255, 100, 255);     // Magenta/pink core
    static constexpr sf::Color COUNTER_TEXT_COLOR = sf::Color(255, 255, 255); // White
    static constexpr sf::Color BARREL_RED = sf::Color(255, 80, 80);        // Bright red for barrel pulse (flame red)
    static constexpr sf::Color LIGHTNING_WHITE = sf::Color(255, 255, 255); // White for lightning glow
    static constexpr sf::Color FLAME_CORE = sf::Color(255, 255, 150);      // Yellow core of flame
    static constexpr sf::Color FLAME_MID = sf::Color(255, 150, 50);        // Orange middle of flame
    static constexpr sf::Color FLAME_OUTER = sf::Color(255, 80, 0);        // Red-orange outer flame

    /**
     * @brief Updates the counter text display.
     */
    void updateCounterText();
    
    /**
     * @brief Updates shooting visual effects.
     * @param deltaTime Time since last update
     */
    void updateShootingEffects(float deltaTime);
    
    /**
     * @brief Renders white lightning glow on cannon boundaries.
     * @param window Render window
     */
    void renderLightningGlow(sf::RenderWindow& window) const;
    
    /**
     * @brief Renders flame/fire effect along cannon contours when shooting.
     * @param window Render window
     */
    void renderFlameEffect(sf::RenderWindow& window) const;
    
    /**
     * @brief Renders flames along base contours.
     * @param window Render window
     * @param intensity Flame intensity (0.0 to 1.0)
     */
    void renderBaseContourFlames(sf::RenderWindow& window, float intensity) const;
    
    /**
     * @brief Renders flames along barrel contours.
     * @param window Render window
     * @param intensity Flame intensity (0.0 to 1.0)
     * @param cosAngle Cosine of barrel angle (unused, kept for future use)
     * @param sinAngle Sine of barrel angle (unused, kept for future use)
     */
    void renderBarrelContourFlames(sf::RenderWindow& window, float intensity, float cosAngle, float sinAngle) const;
    
    /**
     * @brief Renders flames along a rectangle shape contour.
     * @param window Render window
     * @param shape Rectangle shape to render flames around
     * @param intensity Flame intensity (0.0 to 1.0)
     */
    void renderShapeContourFlames(sf::RenderWindow& window, const sf::RectangleShape& shape, float intensity) const;
    
    /**
     * @brief Renders flames along an edge.
     * @param window Render window
     * @param start Edge start position
     * @param end Edge end position
     * @param intensity Flame intensity (0.0 to 1.0)
     * @param isBarrel Whether this is a barrel edge (unused, kept for future differentiation)
     */
    void renderEdgeFlames(sf::RenderWindow& window, const sf::Vector2f& start, const sf::Vector2f& end, float intensity, bool isBarrel) const;

    /**
     * @brief Initializes the base components (stationary).
     */
    void initializeBase();

    /**
     * @brief Initializes the barrel components (rotatable).
     */
    void initializeBarrel();

    /**
     * @brief Updates barrel rotation and position.
     */
    void updateBarrelTransform();

    /**
     * @brief Updates animation state (pulse, glow).
     * @param deltaTime Time elapsed since last frame
     */
    void updateAnimations(float deltaTime);

    /**
     * @brief Renders glow effect for a rectangle shape.
     * @param window Render window
     * @param shape Reference to the rectangle shape to add glow to
     * @param baseColor Base color for glow
     * @param intensity Glow intensity (0.0 to 1.0)
     */
    void renderGlow(sf::RenderWindow& window, const sf::RectangleShape& shape, const sf::Color& baseColor, float intensity) const;

    /**
     * @brief Renders glow effect for a circle shape.
     * @param window Render window
     * @param shape Reference to the circle shape to add glow to
     * @param baseColor Base color for glow
     * @param intensity Glow intensity (0.0 to 1.0)
     */
    void renderGlow(sf::RenderWindow& window, const sf::CircleShape& shape, const sf::Color& baseColor, float intensity) const;

    /**
     * @brief Clamps the angle within bounds.
     */
    void clampAngle();

    /**
     * @brief Converts an angle to a direction vector.
     * @param angle Angle in degrees
     * @return Normalized direction vector
     */
    sf::Vector2f angleToDirection(float angle) const;

    /**
     * @brief Updates the angle from mouse position.
     * @param window Reference to the render window
     */
    void updateAngleFromMouse(const sf::RenderWindow& window);

    /**
     * @brief Converts mouse X position to angle.
     * @param mouseX Mouse X position
     * @param windowWidth Window width
     * @return Angle in degrees
     */
    float mouseXToAngle(float mouseX, float windowWidth) const;
};

#endif // CANNON_H

