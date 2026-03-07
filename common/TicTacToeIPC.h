#ifndef TIC_TAC_TOE_IPC_H
#define TIC_TAC_TOE_IPC_H

#include "GameIPC.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace wizz {

// macOS hard-limits POSIX shm names to 30 characters (including the leading
// '/').  A naive key like "Wizz_IPC_TicTacToe_<roomId>_<username>" easily
// exceeds that, causing shm_open() to fail silently with ENAMETOOLONG.
//
// Solution: hash the full session string (roomId + username) to an 8-char
// hex digest and prefix it with a short tag.  The final key is always 13
// characters ("WTT_" + 8 hex + '\0'), well within the 30-char limit.
inline std::string makeTicTacToeIPCKey(const std::string &session_id) {
  // djb2 hash — fast, simple, collision-resistant enough for this use case
  uint32_t hash = 5381u;
  for (unsigned char c : session_id)
    hash = hash * 33u ^ c;

  std::ostringstream oss;
  oss << "WTT_" << std::hex << std::setw(8) << std::setfill('0') << hash;
  return oss.str(); // e.g. "WTT_a3f92c1b" — always 12 chars
}

// Struct ensuring consistent byte layout between WizzMania (Qt)
// and TicTacToe (SFML) processes.
#pragma pack(push, 1)
struct TicTacToeIPCData {
  bool isMyTurn; // True if the local player can click
  int board[9];  // 0=Empty, 1=X, 2=O

  // Game -> Messenger flag
  bool hasOutboundMove;  // Game sets true when local player clicked
  int outboundCellIndex; // The cell the local player clicked

  // Messenger -> Game flag
  bool hasInboundMove;  // Messenger sets true when opponent packet arrives
  int inboundCellIndex; // Opponent's cell

  // End states
  bool gameOver;
  int winner; // 0=Draw, 1=X, 2=O

  // Rematch request (Game -> Messenger)
  bool rematchRequested; // Player pressed "Play Again" button
  bool quitRequested;    // Player pressed "Quit" button
};
#pragma pack(pop)

} // namespace wizz

#endif // TIC_TAC_TOE_IPC_H
