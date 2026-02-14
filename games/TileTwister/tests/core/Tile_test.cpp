#include "Tile.hpp"
#include <gtest/gtest.h>

// TDD Step 1: Define what we expect 'Tile' to do.

TEST(TileTest, DefaultConstructorShouldBeEmpty) {
  Core::Tile tile;
  EXPECT_TRUE(tile.isEmpty());
  EXPECT_EQ(tile.getValue(), 0);
}

TEST(TileTest, ValueConstructorShouldSetAttributes) {
  Core::Tile tile(2);
  EXPECT_FALSE(tile.isEmpty());
  EXPECT_EQ(tile.getValue(), 2);
}

TEST(TileTest, MergedFlagShouldBeFalseByDefault) {
  Core::Tile tile(2);
  EXPECT_FALSE(tile.hasMerged());
}

TEST(TileTest, SetMergedShouldUpdateState) {
  Core::Tile tile(2);
  tile.setMerged(true);
  EXPECT_TRUE(tile.hasMerged());

  tile.resetMerged();
  EXPECT_FALSE(tile.hasMerged());
}
