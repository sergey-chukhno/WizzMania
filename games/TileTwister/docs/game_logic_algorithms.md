# Tile Twister - Game Logic Algorithms

This document details the core algorithms used in `src/core/GameLogic.cpp` to implement the 2048 game rules.

## 1. The Challenge (DRY Principle)
The 2048 game requires sliding tiles in 4 directions: Up, Down, Left, Right.
Implementing 4 separate loops (one for each direction) is:
1.  **Repetitive**: The merging logic is identical, just the iteration order changes.
2.  **Error-Prone**: Fixing a bug in "Right" might introduce a bug in "Down" if copy-pasted incorrectly.

## 2. The Solution: Transformation Strategy
We implement **ONLY** the `Slide Left` logic. All other directions are handled by temporarily transforming the grid to look like a "Left" slide, processing it, and transforming it back.

### The Transformations
We treat the Grid as a Matrix and use Linear Algebra simplifications.

| Direction | Transformation Steps | Logic |
| :--- | :--- | :--- |
| **Left** | None | `processRow()` |
| **Right** | `Reverse Grid` | `processRow()` |
| **Up** | `Transpose Grid` | `processRow()` |
| **Down** | `Transpose` -> `Reverse` | `processRow()` |

*   **Transpose**: Swaps rows and columns ($A_{ij} \to A_{ji}$).
*   **Reverse**: Reverses the order of tiles in each row ($[A,B,C,D] \to [D,C,B,A]$).

This ensures that our core merging logic is tested and maintained in **one single place**.

---

## 3. Core Algorithm: The "3-Step Shuffle" (`processRow`)
For a single row (e.g., `[2, 0, 2, 2]`), we apply the standard 2048 sliding/merging rules using a robust 3-step approach. This avoids complex pointer arithmetic.

### Step 1: Compression (Input -> Buffer)
We iterate through the row and copy only **non-zero** values into a temporary buffer. This effectively "slides" all tiles to the left, removing gaps.

*   Input: `[2, 0, 2, 2]`
*   Buffer: `[2, 2, 2]`

### Step 2: Merge (Buffer in-place)
We iterate through the buffer. If `buffer[i] == buffer[i+1]`, we merge them.
*   **Rule**: A merged tile doubles in value.
*   **Rule**: The second tile in the pair is consumed (marked as 0).
*   **Rule**: We skip the consumed tile so it doesn't merge again (Single Merge per Turn).

*   Trace:
    *   Compare `Index 0 (2)` and `Index 1 (2)`: Equal!
    *   `Index 0` becomes `4`. `Index 1` becomes `0`.
    *   Skip to `Index 2`.
    *   Buffer State: `[4, 0, 2]`

### Step 3: Reconstruction (Buffer -> Output)
We write the processed buffer back to the original row.
1.  Write all non-zero values.
2.  Fill the remaining slots with `0` (Empty).

*   Writing: `4`, then `2`.
*   Filling: `0`, `0`.
*   **Final Result**: `[4, 2, 0, 0]`

## 4. Complexity Analysis
*   **Space Complexity**: $O(N)$ for the temporary buffer (where N=4). Since N is constant and small, this is effectively purely stack memory ($O(1)$).
*   **Time Complexity**: $O(1)$ (fixed 4x4 grid). The transformation overhead is negligible compared to the development time saved and bugs prevented.
