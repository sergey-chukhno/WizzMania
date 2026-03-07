#pragma once

#include "NativeSharedMemory.h"
#include "TicTacToeIPC.h"
#include "core/states/State.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

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
  void drawBoard(sf::RenderTarget &target);
  void drawHUD(sf::RenderTarget &target);
  void drawSymbol(sf::RenderTarget &target, int x, int y, char sym);
  void checkWinCondition();
  void resetGame();

  // IPC
  std::unique_ptr<wizz::NativeSharedMemory<wizz::TicTacToeIPCData>>
      sharedMemory_;
  std::string ipcKey_;
  wizz::TicTacToeIPCData *ipcData_;

  // Visuals
  sf::Font font_;
  sf::Text titleText_;
  sf::Text statusText_;
  sf::RectangleShape boardBg_;

  // Game state
  bool isMyTurn_;
  int board_[9]; // 0=Empty, 1=X, 2=O
  int mySymVal_;
  int oppSymVal_;
  bool gameOver_;
  int winner_; // 0=none/draw, 1=X, 2=O
};
