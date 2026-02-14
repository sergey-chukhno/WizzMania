#include "core/states/GameOverState.h"
#include "core/FontManager.h"
#include "core/Game.h"
#include "core/states/MenuState.h"
#include "core/states/PlayingState.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GameOverState::GameOverState(Game *game, int score, int level,
                             int bricksDestroyed)
    : game_(game), score_(score), level_(level),
      bricksDestroyed_(bricksDestroyed), font_(FontManager::getBodyFont()),
      displayFont_(FontManager::getDisplayFont()),
      titleText_(displayFont_, "GAME OVER", 64),
      scoreText_(font_, "",
                 36) // Initialize with empty string, will set in initializeUI
      ,
      levelText_(font_, "",
                 28) // Initialize with empty string, will set in initializeUI
      ,
      bricksDestroyedText_(
          font_, "",
          28) // Initialize with empty string, will set in initializeUI
      ,
      buttonLabels_{"RESTART", "MENU"}, titleFloatTime_(0.0f),
      titleColorTime_(0.0f), titleBaseY_(0.0f), titleBasePosition_(0.0f, 0.0f),
      statisticsColorTime_(0.0f), backgroundAnimationTime_(0.0f) {
  initializeUI();
  initializeBackground();
}

void GameOverState::initializeUI() {
  // Title (will be animated with color cycling)
  titleText_.setFillColor(getTitleColorPink()); // Start with pink
  titleText_.setStyle(sf::Text::Bold);

  // Center title and store base position (moved higher)
  // SFML 3.0: Rect uses .size (Vector2f) instead of .width/.height
  sf::FloatRect titleBounds = titleText_.getLocalBounds();
  titleText_.setOrigin(
      sf::Vector2f(titleBounds.size.x / 2.0f, titleBounds.size.y / 2.0f));
  titleBaseY_ = 120.0f; // Moved higher from 200.0f to create more space
  titleBasePosition_ = sf::Vector2f(
      static_cast<float>(game_->getWindowWidth()) / 2.0f, titleBaseY_);
  titleText_.setPosition(titleBasePosition_);

  // Score text (will be animated with color cycling)
  std::stringstream ss;
  ss << "FINAL SCORE: " << score_;
  scoreText_.setString(ss.str());
  scoreText_.setFillColor(getTitleColorCyan()); // Start with cyan
  scoreText_.setStyle(sf::Text::Bold);

  // Center score text (adjusted spacing from title)
  // SFML 3.0: Rect uses .size (Vector2f) instead of .width/.height
  sf::FloatRect scoreBounds = scoreText_.getLocalBounds();
  scoreText_.setOrigin(
      sf::Vector2f(scoreBounds.size.x / 2.0f, scoreBounds.size.y / 2.0f));
  scoreText_.setPosition(
      sf::Vector2f(static_cast<float>(game_->getWindowWidth()) / 2.0f,
                   240.0f // Adjusted from 280.0f (more space from title)
                   ));

  // Level text (will be animated with color cycling)
  std::stringstream levelSs;
  levelSs << "LEVEL REACHED: " << level_;
  levelText_.setString(levelSs.str());
  levelText_.setFillColor(getTitleColorCyan()); // Start with cyan
  levelText_.setStyle(sf::Text::Bold);

  // Center level text
  sf::FloatRect levelBounds = levelText_.getLocalBounds();
  levelText_.setOrigin(
      sf::Vector2f(levelBounds.size.x / 2.0f, levelBounds.size.y / 2.0f));
  levelText_.setPosition(
      sf::Vector2f(static_cast<float>(game_->getWindowWidth()) / 2.0f,
                   290.0f // Adjusted from 330.0f
                   ));

  // Asteroids destroyed text (will be animated with color cycling)
  std::stringstream asteroidsSs;
  asteroidsSs << "ASTEROIDS DESTROYED: " << bricksDestroyed_;
  bricksDestroyedText_.setString(asteroidsSs.str());
  bricksDestroyedText_.setFillColor(getTitleColorCyan()); // Start with cyan
  bricksDestroyedText_.setStyle(sf::Text::Bold);

  // Center asteroids destroyed text
  sf::FloatRect asteroidsBounds = bricksDestroyedText_.getLocalBounds();
  bricksDestroyedText_.setOrigin(sf::Vector2f(asteroidsBounds.size.x / 2.0f,
                                              asteroidsBounds.size.y / 2.0f));
  bricksDestroyedText_.setPosition(
      sf::Vector2f(static_cast<float>(game_->getWindowWidth()) / 2.0f,
                   340.0f // Adjusted from 370.0f
                   ));

  // Create buttons (adjusted spacing)
  float startY = 450.0f; // Adjusted from 480.0f (moved up since statistics are
                         // closer together)
  float centerX = static_cast<float>(game_->getWindowWidth()) / 2.0f;

  for (size_t i = 0; i < buttonLabels_.size(); ++i) {
    // Create button rectangle
    sf::RectangleShape button(sf::Vector2f(BUTTON_WIDTH, BUTTON_HEIGHT));
    button.setFillColor(BUTTON_COLOR);
    button.setOutlineColor(Game::NEON_CYAN); // Cyan outline
    button.setOutlineThickness(2.0f);
    // SFML 3.0: setOrigin and setPosition take Vector2f
    button.setOrigin(sf::Vector2f(BUTTON_WIDTH / 2.0f, BUTTON_HEIGHT / 2.0f));
    button.setPosition(
        sf::Vector2f(centerX, startY + i * (BUTTON_HEIGHT + BUTTON_SPACING)));
    buttons_.push_back(button);

    // Create button text
    // SFML 3.0: Text requires font in constructor
    sf::Text text(displayFont_, buttonLabels_[i], 24);
    text.setFillColor(TEXT_COLOR);
    text.setStyle(sf::Text::Bold);

    // Center text on button
    // SFML 3.0: Rect uses .size (Vector2f) instead of .width/.height
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin(
        sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
    text.setPosition(
        sf::Vector2f(centerX, startY + i * (BUTTON_HEIGHT + BUTTON_SPACING)));
    buttonTexts_.push_back(text);
  }
}

void GameOverState::update(float deltaTime) {
  // Update background animations
  updateBackground(deltaTime);

  // Update title floating and color animations
  updateTitleAnimation(deltaTime);

  // Update statistics color animations
  updateStatisticsAnimations(deltaTime);

  // Update button hover states based on mouse position
  sf::Vector2i mousePixelPos = sf::Mouse::getPosition(game_->getWindow());
  sf::Vector2f mousePos = game_->getWindow().mapPixelToCoords(mousePixelPos);
  updateButtonHover(mousePos);
}

void GameOverState::render(sf::RenderWindow &window) {
  // Render background first
  renderBackground(window);

  // Draw title
  window.draw(titleText_);

  // Draw score
  window.draw(scoreText_);

  // Draw level reached
  window.draw(levelText_);

  // Draw bricks destroyed
  window.draw(bricksDestroyedText_);

  // Draw buttons
  for (size_t i = 0; i < buttons_.size(); ++i) {
    window.draw(buttons_[i]);
    window.draw(buttonTexts_[i]);
  }
}

void GameOverState::handleEvent(const sf::Event &event) {
  if (auto *mouseButton = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mouseButton->button == sf::Mouse::Button::Left) {
      sf::Vector2i mousePixelPos = sf::Mouse::getPosition(game_->getWindow());
      sf::Vector2f mousePos =
          game_->getWindow().mapPixelToCoords(mousePixelPos);
      handleButtonClick(mousePos);
    }
  }

  // Keyboard navigation
  if (auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
    if (keyPressed->code == sf::Keyboard::Key::Enter ||
        keyPressed->code == sf::Keyboard::Key::Space) {
      // Activate first button (RESTART)
      handleButtonClick(
          sf::Vector2f(static_cast<float>(game_->getWindowWidth()) / 2.0f,
                       450.0f // Updated to match new button position
                       ));
    }
  }
}

void GameOverState::onEnter() {
  std::cout << "Entered GameOverState" << std::endl;
}

void GameOverState::onExit() {
  std::cout << "Exited GameOverState" << std::endl;
}

void GameOverState::updateTitleAnimation(float deltaTime) {
  // Update floating animation time
  titleFloatTime_ += deltaTime;

  // Update color animation time
  titleColorTime_ += deltaTime;

  // Calculate floating offset (vertical sine wave)
  float floatOffset = std::sin(TITLE_FLOAT_SPEED * 2.0f *
                               static_cast<float>(M_PI) * titleFloatTime_) *
                      TITLE_FLOAT_AMPLITUDE;

  // Update title position with floating offset
  sf::Vector2f newPosition = titleBasePosition_;
  newPosition.y = titleBaseY_ + floatOffset;
  titleText_.setPosition(newPosition);

  // Cycle through colors: Pink -> Cyan -> Purple -> Green -> Yellow -> Pink
  const int colorCount = 5;
  float colorCycle = std::fmod(titleColorTime_ * TITLE_COLOR_CHANGE_SPEED,
                               static_cast<float>(colorCount));
  int colorIndex1 = static_cast<int>(colorCycle) % colorCount;
  int colorIndex2 = (colorIndex1 + 1) % colorCount;
  float t = colorCycle - static_cast<float>(colorIndex1);

  // Get colors based on index
  sf::Color color1, color2;
  switch (colorIndex1) {
  case 0:
    color1 = getTitleColorPink();
    break;
  case 1:
    color1 = getTitleColorCyan();
    break;
  case 2:
    color1 = getTitleColorPurple();
    break;
  case 3:
    color1 = getTitleColorGreen();
    break;
  case 4:
    color1 = getTitleColorYellow();
    break;
  default:
    color1 = getTitleColorPink();
    break;
  }
  switch (colorIndex2) {
  case 0:
    color2 = getTitleColorPink();
    break;
  case 1:
    color2 = getTitleColorCyan();
    break;
  case 2:
    color2 = getTitleColorPurple();
    break;
  case 3:
    color2 = getTitleColorGreen();
    break;
  case 4:
    color2 = getTitleColorYellow();
    break;
  default:
    color2 = getTitleColorPink();
    break;
  }

  // Interpolate between colors
  sf::Color currentColor = lerpColor(color1, color2, t);
  titleText_.setFillColor(currentColor);
}

sf::Color GameOverState::lerpColor(const sf::Color &color1,
                                   const sf::Color &color2, float t) {
  // Clamp t to [0, 1]
  t = std::max(0.0f, std::min(1.0f, t));

  // Linear interpolation for each color component
  unsigned char r =
      static_cast<unsigned char>(color1.r + (color2.r - color1.r) * t);
  unsigned char g =
      static_cast<unsigned char>(color1.g + (color2.g - color1.g) * t);
  unsigned char b =
      static_cast<unsigned char>(color1.b + (color2.b - color1.b) * t);

  return sf::Color(r, g, b);
}

void GameOverState::updateStatisticsAnimations(float deltaTime) {
  // Update statistics color animation time
  statisticsColorTime_ += deltaTime;

  // Cycle through colors for each statistic with different offsets
  const int colorCount = 5;

  // Score text: cycle through colors
  float scoreColorCycle =
      std::fmod(statisticsColorTime_ * STATISTICS_COLOR_CHANGE_SPEED,
                static_cast<float>(colorCount));
  sf::Color scoreColor = getColorFromPalette(scoreColorCycle);
  scoreText_.setFillColor(scoreColor);

  // Level text: cycle with offset (slightly delayed)
  float levelColorCycle =
      std::fmod(statisticsColorTime_ * STATISTICS_COLOR_CHANGE_SPEED + 1.0f,
                static_cast<float>(colorCount));
  sf::Color levelColor = getColorFromPalette(levelColorCycle);
  levelText_.setFillColor(levelColor);

  // Asteroids text: cycle with different offset
  float asteroidsColorCycle =
      std::fmod(statisticsColorTime_ * STATISTICS_COLOR_CHANGE_SPEED + 2.0f,
                static_cast<float>(colorCount));
  sf::Color asteroidsColor = getColorFromPalette(asteroidsColorCycle);
  bricksDestroyedText_.setFillColor(asteroidsColor);
}

sf::Color GameOverState::getColorFromPalette(float timeOffset) {
  const int colorCount = 5;
  float colorCycle = std::fmod(timeOffset, static_cast<float>(colorCount));
  int colorIndex1 = static_cast<int>(colorCycle) % colorCount;
  int colorIndex2 = (colorIndex1 + 1) % colorCount;
  float t = colorCycle - static_cast<float>(colorIndex1);

  // Get colors based on index
  sf::Color color1, color2;
  switch (colorIndex1) {
  case 0:
    color1 = getTitleColorPink();
    break;
  case 1:
    color1 = getTitleColorCyan();
    break;
  case 2:
    color1 = getTitleColorPurple();
    break;
  case 3:
    color1 = getTitleColorGreen();
    break;
  case 4:
    color1 = getTitleColorYellow();
    break;
  default:
    color1 = getTitleColorPink();
    break;
  }
  switch (colorIndex2) {
  case 0:
    color2 = getTitleColorPink();
    break;
  case 1:
    color2 = getTitleColorCyan();
    break;
  case 2:
    color2 = getTitleColorPurple();
    break;
  case 3:
    color2 = getTitleColorGreen();
    break;
  case 4:
    color2 = getTitleColorYellow();
    break;
  default:
    color2 = getTitleColorPink();
    break;
  }

  // Interpolate between colors
  return lerpColor(color1, color2, t);
}

void GameOverState::initializeBackground() {
  // Initialize random seed
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  float windowWidth = static_cast<float>(game_->getWindowWidth());
  float windowHeight = static_cast<float>(game_->getWindowHeight());

  // Initialize planets
  planets_.clear();
  for (int i = 0; i < PLANET_COUNT; ++i) {
    Planet planet;

    // Random planet properties
    float radius =
        PLANET_MIN_RADIUS + static_cast<float>(std::rand()) /
                                static_cast<float>(RAND_MAX) *
                                (PLANET_MAX_RADIUS - PLANET_MIN_RADIUS);
    planet.shape.setRadius(radius);
    planet.shape.setOrigin(sf::Vector2f(radius, radius));

    // Random planet colors (cyberpunk palette)
    std::vector<sf::Color> planetColors = {
        sf::Color(255, 0, 110), // Pink
        sf::Color(0, 217, 255), // Cyan
        sf::Color(170, 0, 255), // Purple
        sf::Color(0, 255, 136), // Green
        sf::Color(255, 136, 0)  // Orange
    };
    planet.color = planetColors[std::rand() % planetColors.size()];
    planet.shape.setFillColor(planet.color);
    planet.shape.setOutlineColor(
        sf::Color(planet.color.r, planet.color.g, planet.color.b, 100));
    planet.shape.setOutlineThickness(2.0f);

    // Rotation properties
    planet.rotationAngle =
        static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 360.0f;
    planet.rotationSpeed =
        PLANET_MIN_ROTATION_SPEED +
        static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) *
            (PLANET_MAX_ROTATION_SPEED - PLANET_MIN_ROTATION_SPEED);

    // Orbit properties
    planet.orbitCenter = sf::Vector2f(
        windowWidth * (0.2f + static_cast<float>(std::rand()) /
                                  static_cast<float>(RAND_MAX) * 0.6f),
        windowHeight * (0.2f + static_cast<float>(std::rand()) /
                                   static_cast<float>(RAND_MAX) * 0.6f));
    planet.orbitRadius = 50.0f + static_cast<float>(std::rand()) /
                                     static_cast<float>(RAND_MAX) * 100.0f;
    planet.orbitAngle =
        static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 360.0f;
    planet.orbitSpeed = PLANET_MIN_ORBIT_SPEED +
                        static_cast<float>(std::rand()) /
                            static_cast<float>(RAND_MAX) *
                            (PLANET_MAX_ORBIT_SPEED - PLANET_MIN_ORBIT_SPEED);

    // Set initial position
    float orbitX =
        planet.orbitCenter.x +
        std::cos(planet.orbitAngle * static_cast<float>(M_PI) / 180.0f) *
            planet.orbitRadius;
    float orbitY =
        planet.orbitCenter.y +
        std::sin(planet.orbitAngle * static_cast<float>(M_PI) / 180.0f) *
            planet.orbitRadius;
    planet.shape.setPosition(sf::Vector2f(orbitX, orbitY));

    // Randomly assign rings (Jupiter-like) - about 50% chance
    planet.hasRings = (std::rand() % 2 == 0);
    if (planet.hasRings) {
      createPlanetRings(planet, radius);
    }

    // Randomly assign pulsar effect - about 40% chance
    planet.isPulsarPlanet = (std::rand() % 100 < 40);
    if (planet.isPulsarPlanet) {
      // Initialize pulsar glow
      float glowRadius = radius * PLANET_GLOW_MULTIPLIER;
      planet.glow.setRadius(glowRadius);
      planet.glow.setOrigin(sf::Vector2f(glowRadius, glowRadius));
      planet.glow.setFillColor(
          sf::Color(planet.color.r, planet.color.g, planet.color.b, 40));
      planet.glow.setPosition(sf::Vector2f(orbitX, orbitY));
      planet.pulseTime = static_cast<float>(std::rand()) /
                         static_cast<float>(RAND_MAX) *
                         (2.0f * static_cast<float>(M_PI));
      planet.pulseSpeed = PLANET_PULSE_SPEED;
    }

    planets_.push_back(planet);
  }

  // Initialize pulsars
  pulsars_.clear();
  for (int i = 0; i < PULSAR_COUNT; ++i) {
    Pulsar pulsar;

    // Random pulsar properties
    pulsar.baseRadius =
        PULSAR_MIN_RADIUS + static_cast<float>(std::rand()) /
                                static_cast<float>(RAND_MAX) *
                                (PULSAR_MAX_RADIUS - PULSAR_MIN_RADIUS);
    pulsar.glowRadius = pulsar.baseRadius * PULSAR_GLOW_MULTIPLIER;

    // Random position
    pulsar.position = sf::Vector2f(
        windowWidth * (0.1f + static_cast<float>(std::rand()) /
                                  static_cast<float>(RAND_MAX) * 0.8f),
        windowHeight * (0.1f + static_cast<float>(std::rand()) /
                                   static_cast<float>(RAND_MAX) * 0.8f));

    // Pulsar colors (bright cyan/pink)
    std::vector<sf::Color> pulsarColors = {
        sf::Color(0, 217, 255), // Cyan
        sf::Color(255, 0, 110), // Pink
        sf::Color(170, 0, 255)  // Purple
    };
    pulsar.color = pulsarColors[std::rand() % pulsarColors.size()];

    // Initialize shapes
    pulsar.core.setRadius(pulsar.baseRadius);
    pulsar.core.setOrigin(sf::Vector2f(pulsar.baseRadius, pulsar.baseRadius));
    pulsar.core.setFillColor(pulsar.color);
    pulsar.core.setPosition(pulsar.position);

    pulsar.glow.setRadius(pulsar.glowRadius);
    pulsar.glow.setOrigin(sf::Vector2f(pulsar.glowRadius, pulsar.glowRadius));
    pulsar.glow.setFillColor(
        sf::Color(pulsar.color.r, pulsar.color.g, pulsar.color.b, 50));
    pulsar.glow.setPosition(pulsar.position);

    // Pulse properties
    pulsar.pulseTime = static_cast<float>(std::rand()) /
                       static_cast<float>(RAND_MAX) *
                       (2.0f * static_cast<float>(M_PI));
    pulsar.pulseSpeed = PULSAR_PULSE_SPEED;

    pulsars_.push_back(pulsar);
  }
}

void GameOverState::updateBackground(float deltaTime) {
  // Update animation time
  backgroundAnimationTime_ += deltaTime;

  // Update planets
  for (auto &planet : planets_) {
    // Update rotation
    planet.rotationAngle += planet.rotationSpeed * deltaTime;
    if (planet.rotationAngle >= 360.0f) {
      planet.rotationAngle -= 360.0f;
    }
    planet.shape.setRotation(sf::degrees(planet.rotationAngle));

    // Update orbit
    planet.orbitAngle += planet.orbitSpeed * deltaTime;
    if (planet.orbitAngle >= 360.0f) {
      planet.orbitAngle -= 360.0f;
    }

    // Update position based on orbit
    float orbitX =
        planet.orbitCenter.x +
        std::cos(planet.orbitAngle * static_cast<float>(M_PI) / 180.0f) *
            planet.orbitRadius;
    float orbitY =
        planet.orbitCenter.y +
        std::sin(planet.orbitAngle * static_cast<float>(M_PI) / 180.0f) *
            planet.orbitRadius;
    sf::Vector2f planetPosition(orbitX, orbitY);
    planet.shape.setPosition(planetPosition);

    // Update rings position and rotation
    if (planet.hasRings) {
      for (size_t ringIndex = 0; ringIndex < planet.rings.size(); ++ringIndex) {
        auto &ring = planet.rings[ringIndex];
        ring.setPosition(planetPosition);

        // Rings rotate with the planet (slightly slower for visual depth)
        // Add a slight tilt based on ring index to create depth effect
        float baseTilt =
            -10.0f + static_cast<float>(ringIndex) * 5.0f; // Progressive tilt
        float ringRotation =
            planet.rotationAngle * 0.6f + baseTilt; // Slower rotation + tilt
        // Wrap rotation to 0-360 range
        while (ringRotation >= 360.0f)
          ringRotation -= 360.0f;
        while (ringRotation < 0.0f)
          ringRotation += 360.0f;
        ring.setRotation(sf::degrees(ringRotation));
      }
    }

    // Update pulsar planet glow
    if (planet.isPulsarPlanet) {
      // Update pulse time
      planet.pulseTime +=
          planet.pulseSpeed * 2.0f * static_cast<float>(M_PI) * deltaTime;
      if (planet.pulseTime >= 2.0f * static_cast<float>(M_PI)) {
        planet.pulseTime -= 2.0f * static_cast<float>(M_PI);
      }

      // Calculate pulse factor
      float pulseFactor =
          0.5f + 0.5f * (std::sin(planet.pulseTime) + 1.0f) * 0.5f;
      float currentGlowRadius = planet.shape.getRadius() *
                                PLANET_GLOW_MULTIPLIER *
                                (0.7f + pulseFactor * 0.6f);

      // Update glow size and alpha
      planet.glow.setRadius(currentGlowRadius);
      planet.glow.setOrigin(sf::Vector2f(currentGlowRadius, currentGlowRadius));
      planet.glow.setPosition(planetPosition);
      unsigned char glowAlpha =
          static_cast<unsigned char>(30 + pulseFactor * 50);
      planet.glow.setFillColor(
          sf::Color(planet.color.r, planet.color.g, planet.color.b, glowAlpha));
    }
  }

  // Update pulsars
  for (auto &pulsar : pulsars_) {
    // Update pulse time
    pulsar.pulseTime +=
        pulsar.pulseSpeed * 2.0f * static_cast<float>(M_PI) * deltaTime;
    if (pulsar.pulseTime >= 2.0f * static_cast<float>(M_PI)) {
      pulsar.pulseTime -= 2.0f * static_cast<float>(M_PI);
    }

    // Calculate pulse factor (sine wave from -1 to 1, normalized to 0.5 to 1.5)
    float pulseFactor =
        0.5f + 0.5f * (std::sin(pulsar.pulseTime) + 1.0f) * 0.5f;
    float currentRadius =
        pulsar.baseRadius *
        (0.7f + pulseFactor * 0.6f); // Varies from 0.7x to 1.3x
    float currentGlowRadius =
        pulsar.glowRadius *
        (0.6f + pulseFactor * 0.8f); // Varies from 0.6x to 1.4x

    // Update core size
    pulsar.core.setRadius(currentRadius);
    pulsar.core.setOrigin(sf::Vector2f(currentRadius, currentRadius));

    // Update glow size and alpha
    pulsar.glow.setRadius(currentGlowRadius);
    pulsar.glow.setOrigin(sf::Vector2f(currentGlowRadius, currentGlowRadius));
    unsigned char glowAlpha = static_cast<unsigned char>(30 + pulseFactor * 40);
    pulsar.glow.setFillColor(
        sf::Color(pulsar.color.r, pulsar.color.g, pulsar.color.b, glowAlpha));
  }
}

void GameOverState::renderBackground(sf::RenderWindow &window) const {
  // Render pulsars first (behind planets)
  for (const auto &pulsar : pulsars_) {
    window.draw(pulsar.glow);
    window.draw(pulsar.core);
  }

  // Render planets
  for (const auto &planet : planets_) {
    // Render pulsar glow first (behind planet)
    if (planet.isPulsarPlanet) {
      window.draw(planet.glow);
    }

    // Render rings (behind planet but in front of glow)
    if (planet.hasRings) {
      for (const auto &ring : planet.rings) {
        window.draw(ring);
      }
    }

    // Render planet
    window.draw(planet.shape);
  }
}

void GameOverState::createPlanetRings(Planet &planet, float planetRadius) {
  planet.rings.clear();

  // Create horizontal rings (Jupiter-like, elliptical appearance)
  for (int i = 0; i < static_cast<int>(PLANET_RING_COUNT); ++i) {
    sf::RectangleShape ring;

    // Ring dimensions: wider than planet, thin height (elliptical rings)
    float ringOuterRadius = planetRadius + PLANET_RING_SPACING * (i + 1);
    float ringWidth = ringOuterRadius * 2.0f;
    float ringHeight = PLANET_RING_WIDTH;

    ring.setSize(sf::Vector2f(ringWidth, ringHeight));
    ring.setOrigin(sf::Vector2f(ringWidth / 2.0f, ringHeight / 2.0f));

    // Ring color: slightly transparent version of planet color, with variation
    sf::Color ringColor = planet.color;
    // Make rings slightly darker/lighter for variation
    if (i % 2 == 0) {
      ringColor.r = static_cast<unsigned char>(
          std::min(255, static_cast<int>(ringColor.r * 0.8f)));
      ringColor.g = static_cast<unsigned char>(
          std::min(255, static_cast<int>(ringColor.g * 0.8f)));
      ringColor.b = static_cast<unsigned char>(
          std::min(255, static_cast<int>(ringColor.b * 0.8f)));
    }
    ringColor.a = 180; // Semi-transparent
    ring.setFillColor(ringColor);
    ring.setOutlineColor(sf::Color(ringColor.r, ringColor.g, ringColor.b, 100));
    ring.setOutlineThickness(0.5f);

    // Rings are positioned at the planet's center, will be rotated with planet
    ring.setPosition(planet.shape.getPosition());

    // Rings start horizontal (0 degrees), will be rotated by planet rotation
    ring.setRotation(sf::degrees(0.0f));

    planet.rings.push_back(ring);
  }
}

void GameOverState::handleButtonClick(const sf::Vector2f &mousePos) {
  int buttonIndex = getButtonAt(mousePos);
  if (buttonIndex == -1) {
    return;
  }

  switch (buttonIndex) {
  case 0: // RESTART
    std::cout << "Restart button clicked" << std::endl;
    game_->queueStateChange(std::make_unique<PlayingState>(game_));
    break;

  case 1: // MENU
    std::cout << "Menu button clicked" << std::endl;
    game_->queueStateChange(std::make_unique<MenuState>(game_));
    break;

  default:
    break;
  }
}

void GameOverState::updateButtonHover(const sf::Vector2f &mousePos) {
  int hoveredButton = getButtonAt(mousePos);

  for (size_t i = 0; i < buttons_.size(); ++i) {
    if (static_cast<int>(i) == hoveredButton) {
      buttons_[i].setFillColor(BUTTON_HOVER_COLOR);
    } else {
      buttons_[i].setFillColor(BUTTON_COLOR);
    }
  }
}

int GameOverState::getButtonAt(const sf::Vector2f &mousePos) const {
  for (size_t i = 0; i < buttons_.size(); ++i) {
    if (buttons_[i].getGlobalBounds().contains(mousePos)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}
