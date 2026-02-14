#ifndef PERSISTENCE_MANAGER_HPP
#define PERSISTENCE_MANAGER_HPP

#include "../core/Grid.hpp"
#include <string>
#include <vector>

struct ScoreEntry {
  std::string date; // "DD-MM-YYYY HH:MM"
  int score;
};

class PersistenceManager {
public:
  // Game State Persistence
  static bool saveGame(const Core::Grid &grid, int score);
  static bool loadGame(Core::Grid &grid, int &score);
  static bool hasSaveGame();
  static bool deleteSaveGame();
  static bool deleteAchievements();

  // Leaderboard Persistence
  static void saveLeaderboard(const std::vector<ScoreEntry> &entries);
  static std::vector<ScoreEntry> loadLeaderboard();

  // Achievements Persistence
  static void saveAchievements(const std::vector<bool> &unlocked);
  static std::vector<bool> loadAchievements();

  // High Score Helper
  // Checks if score is in Top 5. If so, adds it and saves.
  // Returns true if added.
  static bool checkAndSaveHighScore(int score);

  // Helpers
  static std::string getCurrentDateTime(); // Returns "DD-MM-YYYY HH:MM"

private:
  static const std::string SAVE_FILE;
  static const std::string LEADERBOARD_FILE;
  static const std::string ACHIEVEMENTS_FILE;
};

#endif // PERSISTENCE_MANAGER_HPP
