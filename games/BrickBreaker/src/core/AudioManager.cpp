#include "core/AudioManager.h"

AudioManager &AudioManager::getInstance() {
  static AudioManager instance;
  return instance;
}

AudioManager::AudioManager() : audioEnabled_(true), isMusicPlaying_(false) {}

bool AudioManager::loadMusic(const std::string &filename) {
  if (!music_.openFromFile(filename)) {
    std::cerr << "Failed to load music: " << filename << std::endl;
    return false;
  }
  return true;
}

bool AudioManager::loadSound(const std::string &id,
                             const std::string &filename) {
  sf::SoundBuffer buffer;
  if (!buffer.loadFromFile(filename)) {
    std::cerr << "Failed to load sound: " << filename << std::endl;
    return false;
  }

  soundBuffers_[id] = std::move(buffer);

  // Create a sound instance for this buffer
  sounds_[id] = std::make_unique<sf::Sound>(soundBuffers_[id]);

  return true;
}

void AudioManager::playMusic(bool loop) {
  isMusicPlaying_ = true;
  if (audioEnabled_) {
    music_.setLooping(loop);
    music_.play();
  }
}

void AudioManager::stopMusic() {
  isMusicPlaying_ = false;
  music_.stop();
}

void AudioManager::pauseMusic() { music_.pause(); }

void AudioManager::playSound(const std::string &id) {
  if (!audioEnabled_)
    return;

  auto it = sounds_.find(id);
  if (it != sounds_.end() && it->second) {
    // Randomize pitch slightly for variety (0.9 to 1.1)
    float pitch = 0.9f + static_cast<float>(rand()) /
                             (static_cast<float>(RAND_MAX / 0.2f));
    it->second->setPitch(pitch);
    it->second->play();
  }
}

bool AudioManager::toggleAudio() {
  audioEnabled_ = !audioEnabled_;

  if (audioEnabled_) {
    // Resume music if it was supposed to be playing
    if (isMusicPlaying_) {
      music_.play();
    }
  } else {
    // Mute everything
    music_.pause();
    for (auto &pair : sounds_) {
      if (pair.second) {
        pair.second->stop();
      }
    }
  }

  return audioEnabled_;
}

bool AudioManager::isAudioEnabled() const { return audioEnabled_; }

void AudioManager::setGlobalVolume(float volume) {
  sf::Listener::setGlobalVolume(volume);
}

float AudioManager::getGlobalVolume() const {
  return sf::Listener::getGlobalVolume();
}
