#include "GameLogic.hpp"
#include <algorithm> // For std::reverse
#include <tuple>
#include <vector>

namespace Core {

GameLogic::MoveResult GameLogic::move(Grid &grid, Direction dir) {
  // Pre-move: Reset merge flags
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      grid.getTile(x, y).resetMerged();
    }
  }

  bool changed = false;
  int totalScore = 0;
  std::vector<MoveEvent> events;

  // 1. Transform Grid
  switch (dir) {
  case Direction::Left:
    break;
  case Direction::Right:
    reverseGrid(grid);
    break;
  case Direction::Up:
    transposeGrid(grid);
    break;
  case Direction::Down:
    transposeGrid(grid);
    reverseGrid(grid);
    break;
  }

  // 2. Process Rows
  for (int y = 0; y < 4; ++y) {
    std::array<Tile, 4> tempRow;
    for (int x = 0; x < 4; ++x)
      tempRow[x] = grid.getTile(x, y);

    auto [rowChanged, rowScore, rowMoves] = slideAndMergeRow(tempRow);

    if (rowChanged) {
      changed = true;
      totalScore += rowScore;
      for (int x = 0; x < 4; ++x)
        grid.getTile(x, y) = tempRow[x];

      // Convert RowMoves to Global MoveEvents
      for (const auto &rm : rowMoves) {
        MoveEvent evt;
        evt.type = rm.isMerge ? MoveEvent::Type::Merge : MoveEvent::Type::Slide;
        evt.value = rm.value;
        evt.mergedValue =
            rm.isMerge ? rm.value * 2 : 0; // If merge, value is PRE-merge
                                           // value? Or post? logic below
        // Actually slideAndMergeRow returns 'value' in Move?
        // Let's verify semantics: rowMove.value = The tile value moving.

        int lx1 = rm.fromIndex;
        int lx2 = rm.toIndex;
        int ly = y;

        // Map Local -> Global
        switch (dir) {
        case Direction::Left:
          evt.fromX = lx1;
          evt.fromY = ly;
          evt.toX = lx2;
          evt.toY = ly;
          break;
        case Direction::Right:
          evt.fromX = 3 - lx1;
          evt.fromY = ly;
          evt.toX = 3 - lx2;
          evt.toY = ly;
          break;
        case Direction::Up:
          evt.fromX = ly;
          evt.fromY = lx1;
          evt.toX = ly;
          evt.toY = lx2;
          break;
        case Direction::Down:
          evt.fromX = ly;
          evt.fromY = 3 - lx1;
          evt.toX = ly;
          evt.toY = 3 - lx2;
          break;
        }
        events.push_back(evt);
      }
    }
  }

  // 3. Restore Grid
  switch (dir) {
  case Direction::Left:
    break;
  case Direction::Right:
    reverseGrid(grid);
    break;
  case Direction::Up:
    transposeGrid(grid);
    break;
  case Direction::Down:
    reverseGrid(grid);
    transposeGrid(grid);
    break;
  }
  return {changed, totalScore, events};
}

std::tuple<bool, int, std::vector<GameLogic::RowMove>>
GameLogic::slideAndMergeRow(std::array<Tile, 4> &row) {
  bool rowChanged = false;
  int score = 0;
  std::vector<RowMove> moves;

  struct BufferedTile {
    int val;
    int originalIndex;
  };
  std::vector<BufferedTile> buffer;
  buffer.reserve(4);

  // Phase 1: Compression
  for (int i = 0; i < 4; ++i) {
    if (!row[i].isEmpty()) {
      buffer.push_back({row[i].getValue(), i});
    }
  }

  std::vector<Tile> mergedResult;
  mergedResult.reserve(4);
  int writeIdx = 0;

  for (size_t i = 0; i < buffer.size(); ++i) {
    if (i < buffer.size() - 1 && buffer[i].val == buffer[i + 1].val) {
      // MERGE
      int newVal = buffer[i].val * 2;
      score += newVal;

      Tile t(newVal);
      t.setMerged(true);
      mergedResult.push_back(t);

      // Record Moves
      moves.push_back({buffer[i].originalIndex, writeIdx, true, buffer[i].val});
      moves.push_back(
          {buffer[i + 1].originalIndex, writeIdx, true, buffer[i + 1].val});

      i++; // Skip next
      writeIdx++;
    } else {
      // KEEP
      mergedResult.push_back(Tile(buffer[i].val));

      // Record Move (only if moved)
      if (buffer[i].originalIndex != writeIdx) {
        moves.push_back(
            {buffer[i].originalIndex, writeIdx, false, buffer[i].val});
      } else {
        // Even if it didn't move index, we might want to record it for
        // consistency if we animate everything? Optimisation: Only animate if
        // moved. So if index is same, no animation needed.
      }
      writeIdx++;
    }
  }

  // Phase 3: Fill Empty
  while (mergedResult.size() < 4) {
    mergedResult.push_back(Tile(0));
  }

  // Write Back & Detect Change
  for (int i = 0; i < 4; ++i) {
    if (row[i].getValue() != mergedResult[i].getValue()) {
      rowChanged = true;
    }
    row[i] = mergedResult[i];
  }

  return {rowChanged, score, moves};
}

bool GameLogic::isGameOver(const Grid &grid) const {
  // 1. Check for empty tiles
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      if (grid.getTile(x, y).isEmpty()) {
        return false; // Empty slot exists -> Playable
      }
    }
  }

  // 2. Check for possible merges (Horizontal)
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 3; ++x) {
      if (grid.getTile(x, y).getValue() == grid.getTile(x + 1, y).getValue()) {
        return false; // Merge possible
      }
    }
  }

  // 3. Check for possible merges (Vertical)
  for (int x = 0; x < 4; ++x) {
    for (int y = 0; y < 3; ++y) {
      if (grid.getTile(x, y).getValue() == grid.getTile(x, y + 1).getValue()) {
        return false; // Merge possible
      }
    }
  }

  return true; // Full and no merges
}

void GameLogic::reverseGrid(Grid &grid) {
  for (int y = 0; y < 4; ++y) {
    // We need to swap [x] with [3-x]
    for (int x = 0; x < 2; ++x) {
      std::swap(grid.getTile(x, y), grid.getTile(3 - x, y));
    }
  }
}

void GameLogic::transposeGrid(Grid &grid) {
  for (int y = 0; y < 4; ++y) {
    for (int x = y + 1; x < 4; ++x) {
      std::swap(grid.getTile(x, y), grid.getTile(y, x));
    }
  }
}

} // namespace Core
