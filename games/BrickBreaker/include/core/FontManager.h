#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

/**
 * @brief Simple font manager for loading and managing fonts.
 *
 * For now, it tries to load a system font or creates a fallback.
 * In the future, this will be expanded to load fonts from assets.
 */
class FontManager {
public:
  /**
   * @brief Gets the default font (fallback to body font).
   * @return Reference to the font
   */
  static const sf::Font &getDefaultFont();

  /**
   * @brief Gets the display font (Orbitron).
   * Used for headers, titles, and important UI elements.
   * @return Reference to the display font
   */
  static const sf::Font &getDisplayFont();

  /**
   * @brief Gets the body font (Rajdhani).
   * Used for general text, descriptions, and smaller UI elements.
   * @return Reference to the body font
   */
  static const sf::Font &getBodyFont();

  /**
   * @brief Cleans up the fonts.
   * Should be called before the program exits.
   */
  static void cleanup();

private:
  static std::unique_ptr<sf::Font> displayFont_;
  static std::unique_ptr<sf::Font> bodyFont_;
  static bool fontsLoaded_;

  /**
   * @brief Attempts to load fonts from assets.
   * @return True if fonts were loaded successfully
   */
  static bool loadFonts();
};

#endif // FONTMANAGER_H
