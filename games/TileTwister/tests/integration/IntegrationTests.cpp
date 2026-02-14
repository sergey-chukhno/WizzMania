
#include "GameLogic.hpp"
#include "Grid.hpp"
#include "PersistenceManager.hpp"
#include <cstdio> // For remove()
#include <gtest/gtest.h>

// Test Fixture to handle cleanup
class IntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Ensure clean state before each test
    std::remove("savegame.txt");
    std::remove("leaderboard.txt");
    std::remove("achievements.txt");
  }

  void TearDown() override {
    // Clean up after test
    std::remove("savegame.txt");
    std::remove("leaderboard.txt");
    std::remove("achievements.txt");
  }
};

// 1. Persistence Round-Trip Test
TEST_F(IntegrationTest, PersistenceRoundTrip) {
  // Setup initial state
  Core::Grid originalGrid;
  originalGrid.getTile(0, 0).setValue(2048);
  originalGrid.getTile(0, 1).setValue(1024);
  int originalScore = 12345;

  // Save
  bool saved = PersistenceManager::saveGame(originalGrid, originalScore);
  ASSERT_TRUE(saved);

  // Reset State
  Core::Grid loadedGrid; // Empty
  int loadedScore = 0;

  // Load
  bool success = PersistenceManager::loadGame(loadedGrid, loadedScore);

  ASSERT_TRUE(success);
  EXPECT_EQ(loadedScore, originalScore);
  EXPECT_EQ(loadedGrid.getTile(0, 0).getValue(), 2048);
  EXPECT_EQ(loadedGrid.getTile(0, 1).getValue(), 1024);
  EXPECT_EQ(loadedGrid.getTile(3, 3).getValue(), 0); // Empty should be 0
}

// 2. Gameplay & State Integration
TEST_F(IntegrationTest, GameplayStateIntegration) {
  Core::Grid grid;
  // Setup row: [2][2][0][0]
  grid.getTile(0, 0).setValue(2);
  grid.getTile(1, 0).setValue(2);

  // Logic instance
  Core::GameLogic logic;

  // Simulate Move Left (2+2 -> 4)
  auto result = logic.move(grid, Core::Direction::Left);

  ASSERT_TRUE(result.moved);
  // Grid should be [4][0][0][0] at row 0
  EXPECT_EQ(grid.getTile(0, 0).getValue(), 4);
  EXPECT_EQ(grid.getTile(1, 0).getValue(), 0);
  // Score should merge 2+2=4
  EXPECT_EQ(result.score, 4);
}

// 3. Leaderboard System Integration
TEST_F(IntegrationTest, LeaderboardOrderingAndPersistence) {
  // Clear existing (though SetUp does this)
  // Insert Scores: 100, 500, 300, 50, 1000, 200
  PersistenceManager::checkAndSaveHighScore(100);
  PersistenceManager::checkAndSaveHighScore(500);
  PersistenceManager::checkAndSaveHighScore(300);
  PersistenceManager::checkAndSaveHighScore(50);
  PersistenceManager::checkAndSaveHighScore(1000);
  PersistenceManager::checkAndSaveHighScore(200);

  // Load back
  auto leaderboard = PersistenceManager::loadLeaderboard();

  // Verify Size (Max 5)
  ASSERT_EQ(leaderboard.size(), 5);

  // Verify Order (Descending)
  EXPECT_EQ(leaderboard[0].score, 1000);
  EXPECT_EQ(leaderboard[1].score, 500);
  EXPECT_EQ(leaderboard[2].score, 300);
  EXPECT_EQ(leaderboard[3].score, 200);
  EXPECT_EQ(leaderboard[4].score, 100);

  // Verify 50 is gone
}

// 4. Achievement Persistence
TEST_F(IntegrationTest, AchievementPersistence) {
  std::vector<bool> originalState = {true, false, true};

  PersistenceManager::saveAchievements(originalState);

  auto loadedState = PersistenceManager::loadAchievements();

  ASSERT_EQ(loadedState.size(), 3);
  EXPECT_TRUE(loadedState[0]);
  EXPECT_FALSE(loadedState[1]);
  EXPECT_TRUE(loadedState[2]);
}
