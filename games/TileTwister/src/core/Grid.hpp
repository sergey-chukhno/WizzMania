#pragma once
#include "Tile.hpp"
#include <array>
#include <random>

#include <utility> // For std::pair

namespace Core {

class Grid {
public:
  static constexpr int SIZE = 4;

  Grid();

  // Core Actions
  void reset();

  /**
   * @brief Spawns a new tile (2 or 4) in a random empty slot.
   * @return {x, y} of the spawned tile, or {-1, -1} if full.
   */
  std::pair<int, int> spawnRandomTile();

  /**
   * @brief Access a tile at specific coordinates.
   * @param x Column (0-3)
   * @param y Row (0-3)
   * @return const reference and non-const reference overloads
   */
  [[nodiscard]] const Tile &getTile(int x, int y) const;
  [[nodiscard]] Tile &getTile(int x, int y);

private:
  std::array<std::array<Tile, SIZE>, SIZE> tiles;

  // Random number generation components
  std::mt19937 rng;
};

} // namespace Core
