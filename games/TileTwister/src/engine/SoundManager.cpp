#include "SoundManager.hpp"
#include <iostream>

namespace Engine {

SoundManager::SoundManager() = default;

SoundManager::~SoundManager() { shutdown(); }

bool SoundManager::init() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: "
              << Mix_GetError() << std::endl;
    return false;
  }
  Mix_AllocateChannels(16); // Allocate more channels to prevent cutoff
  m_initialized = true;
  return true;
}

void SoundManager::shutdown() {
  if (m_initialized) {
    for (auto &[id, chunk] : m_soundBank) {
      Mix_FreeChunk(chunk);
    }
    m_soundBank.clear();
    Mix_CloseAudio();
    m_initialized = false;
  }
}

void SoundManager::loadSound(const std::string &id, const std::string &path) {
  if (!m_initialized)
    return;

  Mix_Chunk *chunk = Mix_LoadWAV(path.c_str());
  if (chunk == nullptr) {
    std::cerr << "Failed to load sound '" << id << "' from " << path
              << "! SDL_mixer Error: " << Mix_GetError() << std::endl;
  } else {
    // If overwriting, free old one
    if (m_soundBank.count(id)) {
      Mix_FreeChunk(m_soundBank[id]);
    }
    m_soundBank[id] = chunk;
    std::cout << "Loaded Sound: " << id << std::endl;
  }
}

void SoundManager::play(const std::string &id, int volume, bool allowOverlay) {
  if (m_muted || !m_initialized)
    return;

  auto it = m_soundBank.find(id);
  if (it != m_soundBank.end()) {
    // If not allowing overlay, check if playing?
    // SDL_mixer plays on channels. Checking if a specific chunk is playing is
    // hard without tracking channels. For this simple engine, we blindly play
    // or rely on OneShot for spam control.

    // Set volume for the specific chunk (affects all future plays of this
    // chunk)
    Mix_VolumeChunk(it->second, volume);

    // Play on first free channel
    Mix_PlayChannel(-1, it->second, 0);
  }
}

void SoundManager::playOneShot(const std::string &id, int volume) {
  if (m_playedThisFrame.count(id)) {
    return; // Already played this frame, skip
  }
  play(id, volume);
  m_playedThisFrame.insert(id);
}

void SoundManager::toggleMute() {
  m_muted = !m_muted;
  std::cout << "Sound Muted: " << (m_muted ? "YES" : "NO") << std::endl;
}

void SoundManager::update() { m_playedThisFrame.clear(); }

} // namespace Engine
