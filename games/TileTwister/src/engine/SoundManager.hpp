#pragma once
#include <SDL_mixer.h>
#include <map>
#include <set>
#include <string>

namespace Engine {

class SoundManager {
public:
  SoundManager();
  ~SoundManager();

  // Lifecycle
  bool init();
  void shutdown();

  // Resource Management
  void loadSound(const std::string &id, const std::string &path);

  // Playback
  // volume: 0-128 (MIX_MAX_VOLUME)
  void play(const std::string &id, int volume = 128, bool allowOverlay = true);

  // Spam prevention: Ensures sound plays only once per frame
  void playOneShot(const std::string &id, int volume = 128);

  // System Controls
  void toggleMute();
  void update(); // call start of frame to reset one-shot flags

private:
  std::map<std::string, Mix_Chunk *> m_soundBank;
  std::set<std::string> m_playedThisFrame;
  bool m_muted = false;
  bool m_initialized = false;
};

} // namespace Engine
