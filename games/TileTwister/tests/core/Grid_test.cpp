#include "Grid.hpp"
#include <gtest/gtest.h>

TEST(GridTest, InitializeEmpty) {
  Core::Grid grid;
  // Check all cells are empty
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      EXPECT_TRUE(grid.getTile(x, y).isEmpty());
      EXPECT_EQ(grid.getTile(x, y).getValue(), 0);
    }
  }
}

TEST(GridTest, SpawnRandomTileAddsTile) {
  Core::Grid grid;
  auto [x, y] = grid.spawnRandomTile();

  EXPECT_NE(x, -1);
  EXPECT_NE(y, -1);

  // Count non-empty tiles
  int nonEmptyCount = 0;
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      if (!grid.getTile(x, y).isEmpty()) {
        nonEmptyCount++;
        int val = grid.getTile(x, y).getValue();
        EXPECT_TRUE(val == 2 || val == 4);
      }
    }
  }
  EXPECT_EQ(nonEmptyCount, 1);
}

TEST(GridTest, ResetClearsBoard) {
  Core::Grid grid;
  grid.spawnRandomTile();
  grid.spawnRandomTile();

  grid.reset();

  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      EXPECT_TRUE(grid.getTile(x, y).isEmpty());
    }
  }
}

TEST(GridTest, CantSpawnOnFullBoard) {
  Core::Grid grid;
  // Fill the board logically (simulation)
  // Since we don't have a specific "set" method for testing exposed yet,
  // we just spam spawnRandomTile 16 times.
  for (int i = 0; i < 16; ++i) {
    EXPECT_NE(grid.spawnRandomTile().first, -1);
  }

  // Board should be full now
  EXPECT_EQ(grid.spawnRandomTile().first, -1);
}
