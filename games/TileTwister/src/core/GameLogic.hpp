#pragma once
#include "Grid.hpp"
#include <vector>

namespace Core {

enum class Direction { Up, Down, Left, Right };

class GameLogic {
public:
  struct MoveEvent {
    enum class Type { Slide, Merge, Spawn };
    Type type;
    int fromX, fromY;
    int toX, toY;
    int value;       // For Merge, this is the resulting value
    int mergedValue; 
  };

  struct MoveResult {
    bool moved;
    int score;
    std::vector<MoveEvent> events; // Added events list
  };

  /**
   * @brief Executes a move on the grid.
   *
   * @param grid The game grid to modify.
   * @param dir The direction to slide tiles.
   * @return MoveResult containing success flag and points earned.
   */
  MoveResult move(Grid &grid, Direction dir);

  // Checks if the game is over (no empty slots and no adjacent merges)
  bool isGameOver(const Grid &grid) const;

private:
  // Internal Helper Struct for Row Processing
  struct RowMove {
    int fromIndex;
    int toIndex;
    bool isMerge;
    int value;
  };

  // Helper to process a single row (Slide Left logic)
  // Returns tuple: { changed, score_gained, row_moves }
  std::tuple<bool, int, std::vector<RowMove>>
  slideAndMergeRow(std::array<Tile, 4> &row);

  // Helpers for Transformation Strategy
  void reverseGrid(Grid &grid);
  void transposeGrid(Grid &grid);
};

} // namespace Core
