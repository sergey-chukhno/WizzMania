#ifndef GAME_IPC_H
#define GAME_IPC_H

#include <algorithm>
#include <cstdint>
#include <string>

namespace wizz {

// The exact name used by the OS to map the shared memory segment.
// Kept short because macOS limits POSIX shared memory/semaphore names to 30
// chars.
constexpr const char *SHARED_MEMORY_KEY = "Wizz_IPC";

// Sanitize a username so it can be safely used as part of a POSIX shm_open
// name. POSIX requires the name to start with '/' and contain no slashes or
// any other characters outside the portable filename character set. Spaces in
// particular cause shm_open to fail on macOS.
inline std::string sanitizeIPCKey(const std::string &username) {
  std::string safe = username;
  std::replace_if(
      safe.begin(), safe.end(),
      [](char c) {
        return !std::isalnum(static_cast<unsigned char>(c)) && c != '_' &&
               c != '-';
      },
      '_');
  return safe;
}

// Build a user-scoped IPC key (e.g. "WizzMania_Game_IPC_Bob")
inline std::string makeIPCKey(const std::string &username) {
  return std::string(SHARED_MEMORY_KEY) + "_" + sanitizeIPCKey(username);
}

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
