#include "Grid.hpp"
#include <random>
#include <vector>

namespace Core {

Grid::Grid() {
  // Initialize RNG with a random device seed
  std::random_device rd;
  rng.seed(rd());

  reset();
}

void Grid::reset() {
  for (auto &row : tiles) {
    for (auto &tile : row) {
      tile = Tile(0); // Replace with empty tile
    }
  }
}

const Tile &Grid::getTile(int x, int y) const {
  
  return tiles[y][x]; // Row-major: tiles[row][col] -> tiles[y][x]
}

Tile &Grid::getTile(int x, int y) { return tiles[y][x]; }

std::pair<int, int> Grid::spawnRandomTile() {
  std::vector<std::pair<int, int>> emptySlots;
  for (int y = 0; y < SIZE;
       ++y) { // Assuming SIZE is defined and used for grid dimensions
    for (int x = 0; x < SIZE;
         ++x) { // Assuming SIZE is defined and used for grid dimensions
      if (tiles[y][x].isEmpty()) {
        emptySlots.emplace_back(x, y);
      }
    }
  }

  if (emptySlots.empty()) {
    return {-1, -1};
  }

  // Random selection
  // Using the class's rng for consistency with other methods
  std::uniform_int_distribution<int> slotDist(
      0, static_cast<int>(emptySlots.size()) - 1);
  int index = slotDist(rng);
  auto [x, y] = emptySlots[index];

  // 10% chance for 4, 90% for 2
  // Using the class's rng for consistency with other methods
  std::uniform_int_distribution<int> valueProb(0, 9);
  int value = (valueProb(rng) < 1) ? 4 : 2;

  tiles[y][x] = Tile(value);
  return {x, y};
}

} // namespace Core
