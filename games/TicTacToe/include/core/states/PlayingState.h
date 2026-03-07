#pragma once

#include "NativeSharedMemory.h"
#include "TicTacToeIPC.h"
#include "core/states/State.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

// Simple particle for win/draw celebration effect
struct Particle {
  sf::Vector2f pos;
  sf::Vector2f vel;
  float lifetime; // seconds remaining
  float maxLife;
  sf::Color color;
};

class PlayingState : public State {
public:
  explicit PlayingState(Game *game);
  ~PlayingState() override;

  void onEnter() override;
  void onExit() override;
  void processEvent(const sf::Event &event) override;
  void update(sf::Time dt) override;
  void render(sf::RenderTarget &target) override;

private:
  void drawSymbol(sf::RenderTarget &target, int x, int y, char sym);
  void checkWinCondition();
  void spawnParticles();
  void drawGlowText(sf::RenderTarget &target, sf::Text &text,
                    sf::Color glowColor, int passes = 3);
  void centreText(sf::Text &text, const sf::FloatRect &bounds);

  // IPC
  std::unique_ptr<wizz::NativeSharedMemory<wizz::TicTacToeIPCData>>
      sharedMemory_;
  std::string ipcKey_;
  wizz::TicTacToeIPCData *ipcData_;

  // Visuals
  sf::Font font_;

  // Header bar
  sf::Text titleText_;
  sf::Text vsLabel_;
  sf::Text statusText_;
  sf::Texture avatarTexture_;
  // optional because sf::Sprite requires a texture at construction in SFML 3
  std::optional<sf::CircleShape> avatarSpriteOpt_;
  sf::CircleShape avatarMask_; // circle clip ring
  bool avatarLoaded_ = false;

  // Board
  sf::RectangleShape boardBg_;

  // End-game overlay
  sf::Text resultText_;
  sf::RectangleShape rematchBtn_;
  sf::Text rematchText_;
  sf::RectangleShape quitBtn_;
  sf::Text quitText_;

  // Particles
  std::vector<Particle> particles_;
  bool particlesSpawned_ = false;
  float animClock_ = 0.f; // used for pulsing

  // Game state
  bool isMyTurn_;
  int board_[9]; // 0=Empty, 1=X, 2=O
  int mySymVal_;
  int oppSymVal_;
  bool gameOver_;
  int winner_; // 0=draw, 1=X, 2=O
};
