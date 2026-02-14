#include "PersistenceManager.hpp"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

const std::string PersistenceManager::SAVE_FILE = "savegame.txt";
const std::string PersistenceManager::LEADERBOARD_FILE = "leaderboard.txt";
const std::string PersistenceManager::ACHIEVEMENTS_FILE = "achievements.txt";

std::string PersistenceManager::getCurrentDateTime() {
  auto now = std::time(nullptr);
  auto tm = *std::localtime(&now);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%d-%m-%Y %H:%M");
  return oss.str();
}

bool PersistenceManager::saveGame(const Core::Grid &grid, int score) {
  std::ofstream file(SAVE_FILE);
  if (!file.is_open())
    return false;

  file << "SCORE " << score << "\n";
  file << "GRID\n";
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      file << grid.getTile(x, y).getValue() << (x < 3 ? " " : "");
    }
    file << "\n";
  }
  return true;
}

bool PersistenceManager::loadGame(Core::Grid &grid, int &score) {
  std::ifstream file(SAVE_FILE);
  if (!file.is_open())
    return false;

  std::string line;
  while (std::getline(file, line)) {
    if (line.rfind("SCORE", 0) == 0) {
      std::stringstream ss(line);
      std::string temp;
      ss >> temp >> score;
    } else if (line == "GRID") {
      for (int y = 0; y < 4; ++y) {
        if (!std::getline(file, line))
          break;
        std::stringstream ss(line);
        for (int x = 0; x < 4; ++x) {
          int val;
          ss >> val;
          grid.getTile(x, y).setValue(val);
          grid.getTile(x, y).setMerged(false);
        }
      }
    }
  }
  return true;
}

bool PersistenceManager::hasSaveGame() {
  struct stat buffer;
  return (stat(SAVE_FILE.c_str(), &buffer) == 0);
}

bool PersistenceManager::deleteSaveGame() {
  return remove(SAVE_FILE.c_str()) == 0;
}

bool PersistenceManager::deleteAchievements() {
  return remove(ACHIEVEMENTS_FILE.c_str()) == 0;
}

void PersistenceManager::saveLeaderboard(
    const std::vector<ScoreEntry> &entries) {
  std::ofstream file(LEADERBOARD_FILE);
  if (!file.is_open())
    return;

  for (const auto &entry : entries) {
    // Replace spaces in date for easier parsing? Or read whole line?
    // Date format: DD-MM-YYYY HH:MM (contains space).
    // Let's replace space with '_' for file storage, then revert on load?
    // Or just use fixed width parsing.
    // Let's use '_' separator in file.
    std::string dateSafe = entry.date;
    std::replace(dateSafe.begin(), dateSafe.end(), ' ', '_');
    file << dateSafe << " " << entry.score << "\n";
  }
}

std::vector<ScoreEntry> PersistenceManager::loadLeaderboard() {
  std::vector<ScoreEntry> entries;
  std::ifstream file(LEADERBOARD_FILE);
  if (!file.is_open())
    return entries;

  std::string dateSafe;
  int score;
  while (file >> dateSafe >> score) {
    std::replace(dateSafe.begin(), dateSafe.end(), '_', ' ');
    entries.push_back({dateSafe, score});
  }
  // Ensure sort?
  std::sort(entries.begin(), entries.end(),
            [](const ScoreEntry &a, const ScoreEntry &b) {
              return a.score > b.score;
            });
  return entries;
}

bool PersistenceManager::checkAndSaveHighScore(int score) {
  auto entries = loadLeaderboard();

  // Check if worthy
  bool worthy = false;
  if (entries.size() < 5) {
    worthy = true;
  } else {
    if (score > entries.back().score) {
      worthy = true;
    }
  }

  if (worthy) {
    entries.push_back({getCurrentDateTime(), score});
    std::sort(entries.begin(), entries.end(),
              [](const ScoreEntry &a, const ScoreEntry &b) {
                return a.score > b.score;
              });
    if (entries.size() > 5) {
      entries.resize(5);
    }
    saveLeaderboard(entries);
    return true;
  }
  return false;
}

void PersistenceManager::saveAchievements(const std::vector<bool> &unlocked) {
  std::ofstream file(ACHIEVEMENTS_FILE);
  if (file.is_open()) {
    for (bool val : unlocked) {
      file << (val ? 1 : 0) << "\n";
    }
  }
}

std::vector<bool> PersistenceManager::loadAchievements() {
  std::vector<bool> unlocked(3, false);
  std::ifstream file(ACHIEVEMENTS_FILE);
  if (file.is_open()) {
    for (int i = 0; i < 3; ++i) {
      int val;
      if (file >> val) {
        unlocked[i] = (val == 1);
      }
    }
  }
  return unlocked;
}
