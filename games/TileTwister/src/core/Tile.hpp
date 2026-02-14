#pragma once

namespace Core {

/**
 * @brief Represents a single cell on the 2048 grid.
 *
 * Follows Value Semantics: lightweight, copyable, no dynamic allocation.
 */
class Tile {
public:
  // Default constructor creates an empty tile (value 0)
  Tile();

  // Constructor with explicit value
  explicit Tile(int val);

  // Getters
  [[nodiscard]] int getValue() const;
  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] bool hasMerged() const;

  // Setters
  void setMerged(bool merged);
  void resetMerged();
  void setValue(int val);

private:
  int value;
  bool merged;
};

} // namespace Core
