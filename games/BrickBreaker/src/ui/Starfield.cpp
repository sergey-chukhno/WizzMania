#include "ui/Starfield.h"
#include <algorithm>
#include <cmath>
#include <random>

Starfield::Starfield(unsigned int starCount, const sf::Vector2u& windowSize)
    : windowSize_(windowSize)
    , rng_(std::random_device{}())
{
    stars_.reserve(starCount);
    initializeStars();
}

void Starfield::update(float deltaTime)
{
    for (auto& star : stars_)
    {
        // Update twinkling (alpha oscillation)
        star.alpha += star.twinkleSpeed * star.twinkleDirection * deltaTime;

        // Reverse direction if alpha reaches min or max
        if (star.alpha >= star.maxAlpha)
        {
            star.alpha = star.maxAlpha;
            star.twinkleDirection = -1.0f;
        }
        else if (star.alpha <= star.minAlpha)
        {
            star.alpha = star.minAlpha;
            star.twinkleDirection = 1.0f;
        }

        // Clamp alpha to be safe
        star.alpha = clamp(star.alpha, star.minAlpha, star.maxAlpha);

        // Update position (drift)
        star.position += star.velocity * deltaTime;

        // Wrap around screen edges
        if (star.position.x < 0.0f)
        {
            star.position.x = static_cast<float>(windowSize_.x);
        }
        else if (star.position.x > static_cast<float>(windowSize_.x))
        {
            star.position.x = 0.0f;
        }

        if (star.position.y < 0.0f)
        {
            star.position.y = static_cast<float>(windowSize_.y);
        }
        else if (star.position.y > static_cast<float>(windowSize_.y))
        {
            star.position.y = 0.0f;
        }
    }
}

void Starfield::render(sf::RenderWindow& window) const
{
    for (const auto& star : stars_)
    {
        // Set star color with current alpha
        sf::Color starColor = star.color;
        starColor.a = static_cast<unsigned char>(star.alpha * 255.0f);

        // Create circle shape for this star
        sf::CircleShape starShape(star.radius);
        starShape.setFillColor(starColor);
        starShape.setOrigin(sf::Vector2f(star.radius, star.radius));
        starShape.setPosition(star.position);

        // Draw star
        window.draw(starShape);
    }
}

void Starfield::setWindowSize(const sf::Vector2u& size)
{
    windowSize_ = size;
    
    // Optionally reposition stars if window resizes
    // For now, we'll just update the size and let stars wrap around
}

void Starfield::initializeStars()
{
    stars_.clear();

    for (size_t i = 0; i < DEFAULT_STAR_COUNT; ++i)
    {
        Star star;

        // Random position
        star.position.x = randomFloat(0.0f, static_cast<float>(windowSize_.x));
        star.position.y = randomFloat(0.0f, static_cast<float>(windowSize_.y));

        // Random color (pink or cyan)
        star.color = (i % 2 == 0) ? getPinkColor() : getCyanColor();

        // Random radius
        star.radius = randomFloat(STAR_MIN_RADIUS, STAR_MAX_RADIUS);

        // Random initial alpha
        star.alpha = randomFloat(STAR_MIN_ALPHA, STAR_MAX_ALPHA);

        // Random twinkle speed
        star.twinkleSpeed = randomFloat(STAR_MIN_TWINKLE_SPEED, STAR_MAX_TWINKLE_SPEED);

        // Random twinkle direction (start increasing or decreasing)
        star.twinkleDirection = (i % 3 == 0) ? -1.0f : 1.0f;

        // Alpha bounds
        star.minAlpha = STAR_MIN_ALPHA;
        star.maxAlpha = STAR_MAX_ALPHA;

        // Random drift velocity (slow horizontal and vertical drift)
        float driftSpeed = randomFloat(STAR_DRIFT_SPEED_MIN, STAR_DRIFT_SPEED_MAX);
        float driftAngle = randomFloat(0.0f, 2.0f * 3.14159f);  // Random angle
        star.velocity.x = std::cos(driftAngle) * driftSpeed;
        star.velocity.y = std::sin(driftAngle) * driftSpeed;

        stars_.push_back(star);
    }
}

float Starfield::randomFloat(float min, float max) const
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng_);
}

float Starfield::clamp(float value, float min, float max)
{
    return std::max(min, std::min(max, value));
}

