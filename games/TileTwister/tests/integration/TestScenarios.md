# Integration Test Scenarios

## 1. Persistence Round-Trip Test
**Goal**: Verify that the full game state (Grid layout + Score) is correctly serialized to disk and deserialized back without data loss.
*   **Scenario**:
    1.  Initialize a `Grid` with a specific, non-random pattern (e.g., a "2048" tile at [0,0] and a "1024" tile at [0,1]).
    2.  Set a specific Score (e.g., 12345).
    3.  Call `PersistenceManager::saveGame` (using a test-specific filename if possible, or mocking the path).
    4.  Reset the Grid and Score to zero.
    5.  Call `PersistenceManager::loadGame`.
    6.  **Assertion**: The loaded Grid must match the original pattern, and the Score must be 12345.

## 2. Gameplay & State Integration
**Goal**: Verify that `GameLogic` actions correctly mutate the `Grid` and properly update the `GameState` (simulating a turn).
*   **Scenario**:
    1.  Setup a `Grid` with `[2][2][0][0]` in a row.
    2.  Set Score to 0.
    3.  Execute `GameLogic::move(Left)`.
    4.  **Assertion**:
        *   Grid row becomes `[4][0][0][0]`.
        *   Score becomes 4 (Logic must return the score increase).
        *   Merged flags are correctly reset/set.

## 3. Leaderboard System Integration
**Goal**: Verify that the leaderboard logic correctly handles new scores, maintains sorting, and persists data.
*   **Scenario**:
    1.  Clear existing leaderboard data (delete test file).
    2.  Submit a series of scores: `100`, `500`, `300`, `50`, `1000`, `200`.
    3.  Call `PersistenceManager::checkAndSaveHighScore` for each.
    4.  Load the leaderboard.
    5.  **Assertion**:
        *   List contains exactly 5 entries.
        *   Order is Descending: `1000`, `500`, `300`, `200`, `100`.
        *   Verify the '50' was dropped.

## 4. Achievement Persistence Logic
**Goal**: Verify that unlocked achievements are saved and loaded correctly.
*   **Scenario**:
    1.  Create a state `[true, false, true]` (Unlock Medal and Super Cup).
    2.  Call `PersistenceManager::saveAchievements`.
    3.  Load achievements into a new vector.
    4.  **Assertion**: The loaded vector is exactly `[true, false, true]`.
