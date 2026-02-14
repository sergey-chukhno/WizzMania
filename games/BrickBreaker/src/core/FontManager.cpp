#include "core/FontManager.h"
#include <iostream>

std::unique_ptr<sf::Font> FontManager::displayFont_ = nullptr;
std::unique_ptr<sf::Font> FontManager::bodyFont_ = nullptr;
bool FontManager::fontsLoaded_ = false;

const sf::Font &FontManager::getDefaultFont() { return getBodyFont(); }

const sf::Font &FontManager::getDisplayFont() {
  if (!fontsLoaded_) {
    loadFonts();
  }
  return *displayFont_;
}

const sf::Font &FontManager::getBodyFont() {
  if (!fontsLoaded_) {
    loadFonts();
  }
  return *bodyFont_;
}

bool FontManager::loadFonts() {
  if (fontsLoaded_)
    return true;

  displayFont_ = std::make_unique<sf::Font>();
  bodyFont_ = std::make_unique<sf::Font>();

  bool displayLoaded =
      displayFont_->openFromFile("assets/fonts/Orbitron-Bold.ttf");
  bool bodyLoaded =
      bodyFont_->openFromFile("assets/fonts/Rajdhani-Regular.ttf");

  if (displayLoaded) {
    std::cout << "Loaded Display font: Orbitron-Bold.ttf" << std::endl;
  } else {
    std::cerr << "Failed to load Display font: Orbitron-Bold.ttf" << std::endl;
    // Fallback to system font if possible, or empty font
  }

  if (bodyLoaded) {
    std::cout << "Loaded Body font: Rajdhani-Regular.ttf" << std::endl;
  } else {
    std::cerr << "Failed to load Body font: Rajdhani-Regular.ttf" << std::endl;
  }

  fontsLoaded_ = true;
  return displayLoaded && bodyLoaded;
}

void FontManager::cleanup() {
  if (fontsLoaded_) {
    displayFont_.reset();
    bodyFont_.reset();
    fontsLoaded_ = false;
  }
}
