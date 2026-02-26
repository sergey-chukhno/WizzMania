#ifndef GAME_IPC_H
#define GAME_IPC_H

#include <cstdint>

namespace wizz {

// The exact name used by the OS to map the shared memory segment
constexpr const char *SHARED_MEMORY_KEY = "WizzMania_Game_IPC";

// Struct ensuring consistent byte layout across Qt, SDL2, and SFML processes
#pragma pack(push, 1)
struct GameIPCData {
  bool isPlaying;
  uint32_t currentScore;
  char gameName[32]; // e.g., "BrickBreaker"
};
#pragma pack(pop)

} // namespace wizz

#endif // GAME_IPC_H
