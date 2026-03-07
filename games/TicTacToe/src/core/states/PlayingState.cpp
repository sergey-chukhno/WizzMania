#include "core/states/PlayingState.h"
#include <iostream>

using namespace wizz;

PlayingState::PlayingState(Game *game)
    : State(game), ipcData_(nullptr), titleText_(font_), statusText_(font_),
      resultText_(font_), rematchText_(font_), quitText_(font_),
      isMyTurn_(false), mySymVal_(1), oppSymVal_(2), gameOver_(false),
      winner_(0) {
  for (int i = 0; i < 9; ++i)
    board_[i] = 0;
}

PlayingState::~PlayingState() {}

void PlayingState::onEnter() {
  // Load font - try cyberpunk first, fall back to a system font
  if (!font_.openFromFile("../assets/fonts/Cyberpunk.ttf")) {
    if (!font_.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
      std::cerr << "Warning: Could not load any font.\n";
    }
  }

  titleText_.setFont(font_);
  titleText_.setCharacterSize(28);
  titleText_.setFillColor(sf::Color(0, 255, 255));
  titleText_.setString("TicTacToe vs " + game_->getOpponent());
  titleText_.setPosition(sf::Vector2f(50, 20));

  statusText_.setFont(font_);
  statusText_.setCharacterSize(22);
  statusText_.setPosition(sf::Vector2f(50, 65));

  resultText_.setFont(font_);
  resultText_.setCharacterSize(40);
  resultText_.setPosition(sf::Vector2f(0, 200));

  boardBg_.setSize(sf::Vector2f(400, 400));
  boardBg_.setFillColor(sf::Color(20, 25, 40, 200));
  boardBg_.setOutlineThickness(2);
  boardBg_.setOutlineColor(sf::Color(0, 255, 255, 100));
  boardBg_.setPosition(sf::Vector2f(200, 150));

  // Setup end-game buttons (hidden until gameOver_)
  rematchBtn_.setSize(sf::Vector2f(200, 55));
  rematchBtn_.setFillColor(sf::Color(0, 200, 100, 230));
  rematchBtn_.setOutlineThickness(2);
  rematchBtn_.setOutlineColor(sf::Color(0, 255, 100));
  rematchBtn_.setPosition(sf::Vector2f(150, 460));

  rematchText_.setFont(font_);
  rematchText_.setCharacterSize(22);
  rematchText_.setFillColor(sf::Color::White);
  rematchText_.setString("PLAY AGAIN");

  quitBtn_.setSize(sf::Vector2f(200, 55));
  quitBtn_.setFillColor(sf::Color(200, 30, 60, 230));
  quitBtn_.setOutlineThickness(2);
  quitBtn_.setOutlineColor(sf::Color(255, 50, 80));
  quitBtn_.setPosition(sf::Vector2f(450, 460));

  quitText_.setFont(font_);
  quitText_.setCharacterSize(22);
  quitText_.setFillColor(sf::Color::White);
  quitText_.setString("QUIT");

  // Determine symbol FIRST so isMyTurn_ can be set correctly
  mySymVal_ = (game_->getSymbol() == 'X' || game_->getSymbol() == 'x') ? 1 : 2;
  oppSymVal_ = (mySymVal_ == 1) ? 2 : 1;

  // X always goes first
  isMyTurn_ = (mySymVal_ == 1);

  // Setup IPC - the GAME CREATES the shared memory segment.
  // Key includes username so two game processes on the same machine
  // (local multiplayer test) get separate, non-colliding segments.
  ipcKey_ =
      makeTicTacToeIPCKey(game_->getRoomId() + "_" + game_->getUsername());
  sharedMemory_ =
      std::make_unique<NativeSharedMemory<TicTacToeIPCData>>(ipcKey_);

  if (sharedMemory_->createAndMap()) {
    ipcData_ = sharedMemory_->data();

    sharedMemory_->lock();
    ipcData_->isMyTurn = isMyTurn_;
    ipcData_->hasOutboundMove = false;
    ipcData_->hasInboundMove = false;
    ipcData_->gameOver = false;
    ipcData_->winner = 0;
    ipcData_->rematchRequested = false;
    ipcData_->quitRequested = false;
    for (int i = 0; i < 9; ++i)
      ipcData_->board[i] = 0;
    sharedMemory_->unlock();
  } else {
    std::cerr << "[TicTacToe] WARNING: Failed to create Shared Memory segment!"
                 " Game will run without IPC.\n";
  }
}

void PlayingState::onExit() {}

void PlayingState::processEvent(const sf::Event &event) {
  if (gameOver_) {
    // Handle button clicks on the end-game overlay
    if (const auto *mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
      if (mouseEvent->button == sf::Mouse::Button::Left) {
        sf::Vector2f mp(static_cast<float>(mouseEvent->position.x),
                        static_cast<float>(mouseEvent->position.y));

        if (rematchBtn_.getGlobalBounds().contains(mp)) {
          // Signal the messenger to send a new invite
          if (ipcData_ && sharedMemory_) {
            sharedMemory_->lock();
            ipcData_->rematchRequested = true;
            sharedMemory_->unlock();
          }
        } else if (quitBtn_.getGlobalBounds().contains(mp)) {
          // Signal the bridge to stop before closing
          if (ipcData_ && sharedMemory_) {
            sharedMemory_->lock();
            ipcData_->quitRequested = true;
            sharedMemory_->unlock();
          }
          game_->getWindow().close();
        }
      }
    }
    return;
  }

  if (const auto *mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mouseEvent->button == sf::Mouse::Button::Left) {

      if (!isMyTurn_)
        return;

      int mx = mouseEvent->position.x;
      int my = mouseEvent->position.y;

      // Board is at 200, 150, size 400x400
      // Each cell is 133.3x133.3
      if (mx >= 200 && mx < 600 && my >= 150 && my < 550) {
        int col = (mx - 200) / 133;
        int row = (my - 150) / 133;
        int idx = row * 3 + col;

        if (idx >= 0 && idx < 9 && board_[idx] == 0) {
          // Valid move
          board_[idx] = mySymVal_;
          isMyTurn_ = false;

          // Write to IPC
          if (ipcData_ && sharedMemory_) {
            sharedMemory_->lock();
            ipcData_->board[idx] = mySymVal_;
            ipcData_->outboundCellIndex = idx;
            ipcData_->hasOutboundMove = true;
            sharedMemory_->unlock();
          }

          checkWinCondition();
        }
      }
    }
  }
}

void PlayingState::update(sf::Time /*dt*/) {
  if (!gameOver_) {
    // Poll IPC for opponent moves
    if (ipcData_ && sharedMemory_) {
      sharedMemory_->lock();
      if (ipcData_->hasInboundMove) {
        int opIdx = ipcData_->inboundCellIndex;
        if (opIdx >= 0 && opIdx < 9 && board_[opIdx] == 0) {
          board_[opIdx] = oppSymVal_;
          ipcData_->board[opIdx] = oppSymVal_;
          ipcData_->hasInboundMove = false;
          isMyTurn_ = true;
        }
      }
      sharedMemory_->unlock();
      checkWinCondition();
    }

    if (isMyTurn_) {
      statusText_.setString("> YOUR TURN (" +
                            std::string(1, game_->getSymbol()) + ")_");
      statusText_.setFillColor(sf::Color(0, 255, 100));
    } else {
      statusText_.setString("WAITING FOR OPPONENT...");
      statusText_.setFillColor(sf::Color(255, 100, 100));
    }
  }

  // Position button text centred on each button (done every frame for
  // simplicity as text bounds are stable once set)
  if (gameOver_) {
    auto center = [](sf::Text &txt, const sf::RectangleShape &btn) {
      sf::FloatRect tb = txt.getLocalBounds();
      sf::FloatRect bb = btn.getGlobalBounds();
      txt.setPosition(sf::Vector2f(
          bb.position.x + (bb.size.x - tb.size.x) / 2.f - tb.position.x,
          bb.position.y + (bb.size.y - tb.size.y) / 2.f - tb.position.y));
    };
    center(rematchText_, rematchBtn_);
    center(quitText_, quitBtn_);
  }
}

void PlayingState::checkWinCondition() {
  int winningCombos[8][3] = {
      {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
      {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // cols
      {0, 4, 8}, {2, 4, 6}             // diagonals
  };

  for (auto &combo : winningCombos) {
    if (board_[combo[0]] != 0 && board_[combo[0]] == board_[combo[1]] &&
        board_[combo[1]] == board_[combo[2]]) {
      gameOver_ = true;
      winner_ = board_[combo[0]];
      break;
    }
  }

  // Draw check
  if (!gameOver_) {
    bool draw = true;
    for (int i = 0; i < 9; ++i) {
      if (board_[i] == 0) {
        draw = false;
        break;
      }
    }
    if (draw) {
      gameOver_ = true;
      winner_ = 0;
    }
  }

  if (gameOver_) {
    if (winner_ == mySymVal_) {
      resultText_.setString("YOU WIN! FATALITY.");
      resultText_.setFillColor(sf::Color(0, 255, 200));
      statusText_.setString("VICTORY");
      statusText_.setFillColor(sf::Color(0, 255, 100));
    } else if (winner_ == oppSymVal_) {
      resultText_.setString("YOU LOSE. SYSTEM OFFLINE.");
      resultText_.setFillColor(sf::Color(255, 50, 80));
      statusText_.setString("DEFEAT");
      statusText_.setFillColor(sf::Color(255, 80, 80));
    } else {
      resultText_.setString("DRAW. STALEMATE LOGGED.");
      resultText_.setFillColor(sf::Color(180, 180, 255));
      statusText_.setString("DRAW");
      statusText_.setFillColor(sf::Color(180, 180, 255));
    }

    // Centre the result text horizontally
    sf::FloatRect rb = resultText_.getLocalBounds();
    resultText_.setPosition(
        sf::Vector2f((800.f - rb.size.x) / 2.f - rb.position.x, 200.f));

    // Notify IPC so the client bridge can stop cleanly
    if (ipcData_ && sharedMemory_) {
      sharedMemory_->lock();
      ipcData_->gameOver = true;
      ipcData_->winner = winner_;
      sharedMemory_->unlock();
    }
  }
}

void PlayingState::render(sf::RenderTarget &target) {
  target.draw(titleText_);
  target.draw(statusText_);
  target.draw(boardBg_);

  // Draw Grid Lines
  sf::RectangleShape lineV(sf::Vector2f(2, 400));
  lineV.setFillColor(sf::Color(0, 255, 255, 150));
  lineV.setPosition(sf::Vector2f(333, 150));
  target.draw(lineV);
  lineV.setPosition(sf::Vector2f(466, 150));
  target.draw(lineV);

  sf::RectangleShape lineH(sf::Vector2f(400, 2));
  lineH.setFillColor(sf::Color(0, 255, 255, 150));
  lineH.setPosition(sf::Vector2f(200, 283));
  target.draw(lineH);
  lineH.setPosition(sf::Vector2f(200, 416));
  target.draw(lineH);

  // Draw Symbols
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      int idx = row * 3 + col;
      if (board_[idx] == 1) { // X
        drawSymbol(target, 200 + col * 133, 150 + row * 133, 'X');
      } else if (board_[idx] == 2) { // O
        drawSymbol(target, 200 + col * 133, 150 + row * 133, 'O');
      }
    }
  }

  // End-game overlay
  if (gameOver_) {
    // Dim the board slightly
    sf::RectangleShape overlay(sf::Vector2f(800, 600));
    overlay.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(overlay);

    target.draw(resultText_);
    target.draw(rematchBtn_);
    target.draw(rematchText_);
    target.draw(quitBtn_);
    target.draw(quitText_);
  }
}

void PlayingState::drawSymbol(sf::RenderTarget &target, int x, int y,
                              char sym) {
  sf::Text text(font_);
  text.setCharacterSize(80);
  text.setString(std::string(1, sym));

  if (sym == 'X') {
    text.setFillColor(sf::Color(255, 50, 150)); // Cyberpunk Pink
  } else {
    text.setFillColor(sf::Color(50, 255, 150)); // Cyberpunk Green
  }

  // Center it in the 133x133 cell
  sf::FloatRect bounds = text.getLocalBounds();
  text.setPosition(
      sf::Vector2f(x + (133 - bounds.size.x) / 2.0f - bounds.position.x,
                   y + (133 - bounds.size.y) / 2.0f - bounds.position.y));

  target.draw(text);
}
