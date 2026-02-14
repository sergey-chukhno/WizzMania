#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SFML/Audio.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <string>

/**
 * @brief Singleton class to manage audio (music and sound effects).
 */
class AudioManager {
public:
  /**
   * @brief Gets the singleton instance.
   * @return Reference to the AudioManager instance
   */
  static AudioManager &getInstance();

  /**
   * @brief Loads background music from a file.
   * @param filename Path to the music file
   * @return True if loaded successfully, false otherwise
   */
  bool loadMusic(const std::string &filename);

  /**
   * @brief Loads a sound effect from a file.
   * @param id Unique identifier for the sound
   * @param filename Path to the sound file
   * @return True if loaded successfully, false otherwise
   */
  bool loadSound(const std::string &id, const std::string &filename);

  /**
   * @brief Plays the background music.
   * @param loop Whether to loop the music
   */
  void playMusic(bool loop = true);

  /**
   * @brief Stops the background music.
   */
  void stopMusic();

  /**
   * @brief Pauses the background music.
   */
  void pauseMusic();

  /**
   * @brief Plays a sound effect.
   * @param id Identifier of the sound to play
   */
  void playSound(const std::string &id);

  /**
   * @brief Toggles audio (mute/unmute).
   * @return True if audio is now enabled, false if muted
   */
  bool toggleAudio();

  /**
   * @brief Checks if audio is enabled.
   * @return True if enabled, false otherwise
   */
  bool isAudioEnabled() const;

  /**
   * @brief Sets the global volume (0-100).
   * @param volume Volume level
   */
  void setGlobalVolume(float volume);

  /**
   * @brief Gets the global volume (0-100).
   * @return Volume level
   */
  float getGlobalVolume() const;

private:
  AudioManager();
  ~AudioManager() = default;
  AudioManager(const AudioManager &) = delete;
  AudioManager &operator=(const AudioManager &) = delete;

  sf::Music music_;
  std::map<std::string, sf::SoundBuffer> soundBuffers_;
  std::map<std::string, std::unique_ptr<sf::Sound>> sounds_;

  bool audioEnabled_;
  bool isMusicPlaying_; // Track if music *should* be playing (for unmuting)
};

#endif // AUDIO_MANAGER_H
