#ifndef ANIMATEDTEXT_H
#define ANIMATEDTEXT_H

#include <SFML/Graphics.hpp>
#include <string>

/**
 * @brief Text component with pulsing glow effect.
 * 
 * AnimatedText provides text rendering with:
 * - Pulsing glow effect (alpha oscillation)
 * - Shadow effect (dark offset version)
 * - Multi-layer glow rendering
 * - Configurable pulse speed and range
 */
class AnimatedText
{
public:
    /**
     * @brief Constructs an AnimatedText.
     * @param font Reference to the font for text
     * @param text Text string to display
     * @param characterSize Character size in pixels
     */
    AnimatedText(const sf::Font& font, const std::string& text,
                 unsigned int characterSize);

    /**
     * @brief Updates text animations (pulsing).
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the text with glow and shadow effects.
     * @param window Render window to draw to
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Sets the text position.
     * @param position New position
     */
    void setPosition(const sf::Vector2f& position);

    /**
     * @brief Sets the text string.
     * @param text New text string
     */
    void setString(const std::string& text);

    /**
     * @brief Sets the text fill color.
     * @param color New fill color
     */
    void setFillColor(const sf::Color& color);

    /**
     * @brief Sets the character size.
     * @param size New character size
     */
    void setCharacterSize(unsigned int size);

    /**
     * @brief Sets the pulse speed (alpha change per second).
     * @param speed Pulse speed
     */
    void setPulseSpeed(float speed);

    /**
     * @brief Sets the pulse alpha range.
     * @param minAlpha Minimum alpha (0.0 to 1.0)
     * @param maxAlpha Maximum alpha (0.0 to 1.0)
     */
    void setPulseRange(float minAlpha, float maxAlpha);

    /**
     * @brief Enables or disables the glow effect.
     * @param enabled True to enable, false to disable
     */
    void setGlowEnabled(bool enabled);

    /**
     * @brief Enables or disables the shadow effect.
     * @param enabled True to enable, false to disable
     */
    void setShadowEnabled(bool enabled);

    /**
     * @brief Gets the local bounding rectangle of the text.
     * @return Local bounds
     */
    sf::FloatRect getLocalBounds() const;

    /**
     * @brief Sets the origin of the text.
     * @param origin New origin
     */
    void setOrigin(const sf::Vector2f& origin);

private:
    sf::Text text_;                 // Main text object
    sf::Text shadowText_;           // Shadow text (offset, darker)
    float currentAlpha_;            // Current alpha (0.8 to 1.0)
    float pulseSpeed_;              // Alpha change per second
    float minAlpha_;                // Minimum alpha (0.8)
    float maxAlpha_;                // Maximum alpha (1.0)
    float pulseDirection_;          // 1.0 (increasing) or -1.0 (decreasing)
    bool glowEnabled_;              // Enable glow effect
    bool shadowEnabled_;            // Enable shadow effect
    sf::Vector2f shadowOffset_;     // Shadow offset (2-3px)
    sf::Color baseColor_;           // Base text color

    // Constants
    static constexpr float TEXT_PULSE_MIN_ALPHA = 0.8f;
    static constexpr float TEXT_PULSE_MAX_ALPHA = 1.0f;
    static constexpr float TEXT_PULSE_SPEED = 1.0f;  // Alpha units per second
    static constexpr float SHADOW_OFFSET_X = 3.0f;
    static constexpr float SHADOW_OFFSET_Y = 3.0f;
    static constexpr int GLOW_LAYER_COUNT = 3;

    /**
     * @brief Updates the pulse animation.
     * @param deltaTime Time elapsed since last frame
     */
    void updatePulse(float deltaTime);

    /**
     * @brief Renders the glow effect (multiple layers).
     * @param window Render window to draw to
     */
    void renderGlow(sf::RenderWindow& window) const;

    /**
     * @brief Clamps a value between min and max.
     * @param value Value to clamp
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    static float clamp(float value, float min, float max);
};

#endif // ANIMATEDTEXT_H

