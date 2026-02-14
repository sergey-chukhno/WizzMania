#include "GameLogic.hpp"
#include "Grid.hpp"
#include <gtest/gtest.h>

// Helper to set a row for easy testing
void setRow(Core::Grid &grid, int rowY, std::vector<int> values) {
  for (int x = 0; x < 4; ++x) {
    if (x < values.size()) {
      grid.getTile(x, rowY) = Core::Tile(values[x]);
    } else {
      grid.getTile(x, rowY) = Core::Tile(0);
    }
  }
}

// Helper to check a row
void checkRow(const Core::Grid &grid, int rowY, std::vector<int> expected) {
  for (int x = 0; x < 4; ++x) {
    int val = (x < expected.size()) ? expected[x] : 0;
    EXPECT_EQ(grid.getTile(x, rowY).getValue(), val)
        << "Mismatch at Row " << rowY << ", Col " << x;
  }
}

class GameLogicTest : public ::testing::Test {
protected:
  Core::Grid grid;
  Core::GameLogic logic;
};

TEST_F(GameLogicTest, SlideLeft_SimpleSlide) {
  // [0, 2, 0, 4] -> [2, 4, 0, 0] (No merge, just move)
  setRow(grid, 0, {0, 2, 0, 4});

  auto result = logic.move(grid, Core::Direction::Left);
  EXPECT_TRUE(result.moved);
  checkRow(grid, 0, {2, 4, 0, 0});
}

TEST_F(GameLogicTest, SlideLeft_SimpleMerge) {
  // [2, 2, 0, 0] -> [4, 0, 0, 0]
  setRow(grid, 0, {2, 2, 0, 0});

  auto result = logic.move(grid, Core::Direction::Left);

  EXPECT_TRUE(result.moved);
  checkRow(grid, 0, {4, 0, 0, 0});
  EXPECT_TRUE(grid.getTile(0, 0).hasMerged());
}

TEST_F(GameLogicTest, SlideLeft_MergePriority) {
  // [2, 2, 2, 0] -> [4, 2, 0, 0] NOT [2, 4, 0, 0]
  setRow(grid, 0, {2, 2, 2, 0});

  logic.move(grid, Core::Direction::Left);

  checkRow(grid, 0, {4, 2, 0, 0});
}

TEST_F(GameLogicTest, SlideLeft_DoubleMerge) {
  // [2, 2, 2, 2] -> [4, 4, 0, 0]
  setRow(grid, 0, {2, 2, 2, 2});

  logic.move(grid, Core::Direction::Left);

  checkRow(grid, 0, {4, 4, 0, 0});
}

TEST_F(GameLogicTest, SlideAndMerge) {
  // [2, 0, 2, 2] -> [4, 2, 0, 0]
  // 1. Slide: [2, 2, 2, 0]
  // 2. Merge: [4, 2, 0, 0]
  setRow(grid, 0, {2, 0, 2, 2});

  logic.move(grid, Core::Direction::Left);

  checkRow(grid, 0, {4, 2, 0, 0});
}

TEST_F(GameLogicTest, SlideRight_ReverseLogic) {
  // [2, 2, 0, 0] -> [0, 0, 0, 4]
  setRow(grid, 0, {2, 2, 0, 0});

  auto result = logic.move(grid, Core::Direction::Right);

  EXPECT_TRUE(result.moved);
  checkRow(grid, 0, {0, 0, 0, 4});
}

TEST_F(GameLogicTest, SlideUp_TransposeLogic) {
  // Col 0: [2, 2, 0, 0]T -> [4, 0, 0, 0]T
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(0, 1) = Core::Tile(2);

  auto result = logic.move(grid, Core::Direction::Up);

  EXPECT_TRUE(result.moved);
  EXPECT_EQ(grid.getTile(0, 0).getValue(), 4);
  EXPECT_EQ(grid.getTile(0, 1).getValue(), 0);
}

TEST_F(GameLogicTest, NoMoveReturnsFalse) {
  // [2, 4, 8, 16] -> No moves possible Left
  setRow(grid, 0, {2, 4, 8, 16});

  auto result = logic.move(grid, Core::Direction::Left);

  EXPECT_FALSE(result.moved);
}

TEST_F(GameLogicTest, SlideRight_Merge) {
  // [2, 2, 0, 0] -> [0, 0, 0, 4]
  setRow(grid, 0, {2, 2, 0, 0});

  auto result = logic.move(grid, Core::Direction::Right);

  EXPECT_TRUE(result.moved);
  checkRow(grid, 0, {0, 0, 0, 4});
}

TEST_F(GameLogicTest, SlideDown_Merge) {
  // Col 0: [2, 2, 0, 0]T (Top is 0,0) -> [0, 0, 0, 4]T (Bottom is 3,0)
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(0, 1) = Core::Tile(2);
  grid.getTile(0, 2) = Core::Tile(0);
  grid.getTile(0, 3) = Core::Tile(0);

  auto result = logic.move(grid, Core::Direction::Down);

  EXPECT_TRUE(result.moved);
  // After Down move, 4 should be at (0, 3)
  EXPECT_EQ(grid.getTile(0, 3).getValue(), 4);
  EXPECT_EQ(grid.getTile(0, 2).getValue(), 0);
  EXPECT_EQ(grid.getTile(0, 0).getValue(), 0);
}

TEST_F(GameLogicTest, SlideDown_Complex) {
  // Col 0: [2, 0, 2, 2] -> [0, 2, 0, 4] ? No:
  // Compress Down: [0, 2, 2, 2]
  // Merge Down:    [0, 0, 2, 4]

  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(0, 1) = Core::Tile(0);
  grid.getTile(0, 2) = Core::Tile(2);
  grid.getTile(0, 3) = Core::Tile(2);

  auto result = logic.move(grid, Core::Direction::Down);

  EXPECT_TRUE(result.moved);
  EXPECT_EQ(grid.getTile(0, 3).getValue(), 4);
  EXPECT_EQ(grid.getTile(0, 2).getValue(), 2);
  EXPECT_EQ(grid.getTile(0, 1).getValue(), 0);
}

TEST_F(GameLogicTest, MoveLeft_BasicSlide) {
  // [0 2 0 4] -> [2 4 0 0]
  grid.getTile(1, 0) = Core::Tile(2);
  grid.getTile(3, 0) = Core::Tile(4);

  auto result = logic.move(grid, Core::Direction::Left);
  EXPECT_TRUE(result.moved);
  EXPECT_EQ(result.score, 0); // No merge

  EXPECT_EQ(grid.getTile(0, 0).getValue(), 2);
  EXPECT_EQ(grid.getTile(1, 0).getValue(), 4);
  EXPECT_EQ(grid.getTile(2, 0).getValue(), 0);
}

TEST_F(GameLogicTest, MoveLeft_Merge) {
  // [2 2 0 0] -> [4 0 0 0]
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(1, 0) = Core::Tile(2);

  auto result = logic.move(grid, Core::Direction::Left);
  EXPECT_TRUE(result.moved);
  EXPECT_EQ(result.score, 4); // 2+2=4

  EXPECT_EQ(grid.getTile(0, 0).getValue(), 4);
  EXPECT_TRUE(grid.getTile(0, 0).hasMerged());
}

TEST_F(GameLogicTest, MoveRight_BasicSlide) {
  // [2 4 0 0] -> [0 0 2 4]
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(1, 0) = Core::Tile(4);

  auto result = logic.move(grid, Core::Direction::Right);
  EXPECT_TRUE(result.moved);

  EXPECT_EQ(grid.getTile(2, 0).getValue(), 2);
  EXPECT_EQ(grid.getTile(3, 0).getValue(), 4);
}

TEST_F(GameLogicTest, MoveUp_Merge) {
  // [2]
  // [2] -> [4]
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(0, 1) = Core::Tile(2);

  auto result = logic.move(grid, Core::Direction::Up);
  EXPECT_TRUE(result.moved);
  EXPECT_EQ(result.score, 4);

  EXPECT_EQ(grid.getTile(0, 0).getValue(), 4);
}

TEST_F(GameLogicTest, MoveDown_Slide) {
  // [2]
  // [0]
  // [0]
  // [0] -> [0]...[2]
  grid.getTile(0, 0) = Core::Tile(2);

  auto result = logic.move(grid, Core::Direction::Down);
  EXPECT_TRUE(result.moved);

  EXPECT_EQ(grid.getTile(0, 3).getValue(), 2);
}

TEST_F(GameLogicTest, NoMove) {
  // [2 4 8 16] -> No move possible left
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(1, 0) = Core::Tile(4);
  grid.getTile(2, 0) = Core::Tile(8);
  grid.getTile(3, 0) = Core::Tile(16);

  auto result = logic.move(grid, Core::Direction::Left);
  EXPECT_FALSE(result.moved);
  EXPECT_EQ(result.score, 0);
}

TEST_F(GameLogicTest, ComplexMerge_4444) {
  // [4 4 4 4] -> [8 8 0 0] (Left)
  for (int i = 0; i < 4; ++i)
    grid.getTile(i, 0) = Core::Tile(4);

  auto result = logic.move(grid, Core::Direction::Left);
  EXPECT_TRUE(result.moved);
  EXPECT_EQ(result.score, 16); // 4+4=8, 4+4=8. Total 16.

  EXPECT_EQ(grid.getTile(0, 0).getValue(), 8);
  EXPECT_EQ(grid.getTile(1, 0).getValue(), 8);
  EXPECT_EQ(grid.getTile(2, 0).getValue(), 0);
}

TEST_F(GameLogicTest, ComplexMerge_224) {
  // [2 2 4 0] -> [4 4 0 0]
  grid.getTile(0, 0) = Core::Tile(2);
  grid.getTile(1, 0) = Core::Tile(2);
  grid.getTile(2, 0) = Core::Tile(4);

  auto result = logic.move(grid, Core::Direction::Left);
  EXPECT_TRUE(result.moved);
  EXPECT_EQ(result.score, 4); // Only 2+2=4 merges. The existing 4 just moves.

  EXPECT_EQ(grid.getTile(0, 0).getValue(), 4);
  EXPECT_EQ(grid.getTile(1, 0).getValue(), 4);
  EXPECT_EQ(grid.getTile(0, 1).getValue(), 0);
}

TEST_F(GameLogicTest, GameOver_EmptySlots) {
  // [2 0 0 0] -> Not over
  grid.getTile(0, 0) = Core::Tile(2);
  EXPECT_FALSE(logic.isGameOver(grid));
}

TEST_F(GameLogicTest, GameOver_FullButMergeHorizontal) {
  // [2 2 4 8] -> Merge possible (2-2)
  setRow(grid, 0, {2, 2, 4, 8});
  setRow(grid, 1, {16, 32, 64, 128});
  setRow(grid, 2, {256, 512, 1024, 2048});
  setRow(grid, 3, {2, 4, 8, 16});

  EXPECT_FALSE(logic.isGameOver(grid));
}

TEST_F(GameLogicTest, GameOver_FullButMergeVertical) {
  // [2 4 8 16]
  // [2 32 64 128] -> Merge possible (2-2 vertical)
  setRow(grid, 0, {2, 4, 8, 16});
  setRow(grid, 1, {2, 32, 64, 128});
  setRow(grid, 2, {256, 512, 1024, 2048});
  setRow(grid, 3, {2, 4, 8, 16});

  EXPECT_FALSE(logic.isGameOver(grid));
}

TEST_F(GameLogicTest, GameOver_True) {
  // [2 4 2 4]
  // [4 2 4 2]
  // [2 4 2 4]
  // [4 2 4 2] -> Checkerboard pattern, no merges
  setRow(grid, 0, {2, 4, 2, 4});
  setRow(grid, 1, {4, 2, 4, 2});
  setRow(grid, 2, {2, 4, 2, 4});
  setRow(grid, 3, {4, 2, 4, 2});

  EXPECT_TRUE(logic.isGameOver(grid));
}
