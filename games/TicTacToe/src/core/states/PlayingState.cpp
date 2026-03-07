#include "core/states/PlayingState.h"
#include "core/Game.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace wizz;

// ─── helpers
// ──────────────────────────────────────────────────────────────────

static sf::Color lerp(sf::Color a, sf::Color b, float t) {
  return sf::Color(static_cast<uint8_t>(a.r + (b.r - a.r) * t),
                   static_cast<uint8_t>(a.g + (b.g - a.g) * t),
                   static_cast<uint8_t>(a.b + (b.b - a.b) * t),
                   static_cast<uint8_t>(a.a + (b.a - a.a) * t));
}

// ─── Layout constants
// ───────────────────────────────────────────────────────── Window: 800 × 610
// Header bar: y 0..99 (100 px)
// Board: x 100..700 (600 px wide), y 105..695 ← board is 600×600? let's keep
// 400 px
//   Board x: 200..600 (400 px), y 120..520 (400 px)
// Status bar: y 525..545
// Buttons: y 555..610

namespace Layout {
constexpr float HEADER_H = 100.f;
constexpr float BOARD_X = 200.f;
constexpr float BOARD_Y = 115.f;
constexpr float BOARD_SIZE = 400.f;
constexpr float CELL_SIZE = BOARD_SIZE / 3.f; // ~133.3
constexpr float STATUS_Y = 527.f;
constexpr float BTN_Y = 555.f;
constexpr float BTN_H = 48.f;
constexpr float WIN_W = 800.f;
} // namespace Layout

// ─── Constructor
// ──────────────────────────────────────────────────────────────

PlayingState::PlayingState(Game *game)
    : State(game), ipcData_(nullptr), titleText_(font_), vsLabel_(font_),
      statusText_(font_), resultText_(font_), rematchText_(font_),
      quitText_(font_), isMyTurn_(false), mySymVal_(1), oppSymVal_(2),
      gameOver_(false), winner_(0) {
  for (int i = 0; i < 9; ++i)
    board_[i] = 0;
}

PlayingState::~PlayingState() {}

// ─── onEnter
// ──────────────────────────────────────────────────────────────────

void PlayingState::onEnter() {
  // ── Font ──────────────────────────────────────────────────────────────────
  bool fontOk = font_.openFromFile("assets/fonts/Orbitron.ttf");
  if (!fontOk)
    fontOk = font_.openFromFile("../assets/fonts/Orbitron.ttf");
  if (!fontOk)
    font_.openFromFile("/System/Library/Fonts/Helvetica.ttc");

  // ── Title bar texts ───────────────────────────────────────────────────────
  titleText_.setFont(font_);
  titleText_.setCharacterSize(20);
  titleText_.setFillColor(sf::Color(0, 220, 255));
  titleText_.setString("PLAYING TICTACTOE AGAINST");
  titleText_.setPosition(sf::Vector2f(110.f, 12.f));

  vsLabel_.setFont(font_);
  vsLabel_.setCharacterSize(26);
  vsLabel_.setFillColor(sf::Color::White);
  vsLabel_.setStyle(sf::Text::Bold);
  vsLabel_.setString(game_->getOpponent());
  vsLabel_.setPosition(sf::Vector2f(110.f, 40.f));

  statusText_.setFont(font_);
  statusText_.setCharacterSize(17);
  statusText_.setPosition(sf::Vector2f(Layout::BOARD_X, Layout::STATUS_Y));

  // ── Opponent avatar ───────────────────────────────────────────────────────
  const std::string &ap = game_->getAvatarPath();
  if (!ap.empty() && avatarTexture_.loadFromFile(ap)) {
    avatarLoaded_ = true;
    avatarSpriteOpt_.emplace(avatarTexture_);
    // Scale to 72×72
    auto sz = avatarTexture_.getSize();
    float scale = 72.f / std::min((float)sz.x, (float)sz.y);
    avatarSpriteOpt_->setScale(sf::Vector2f(scale, scale));
    avatarSpriteOpt_->setPosition(sf::Vector2f(20.f, 14.f));
  }

  // Circle ring around avatar area
  avatarMask_.setRadius(38.f);
  avatarMask_.setFillColor(sf::Color::Transparent);
  avatarMask_.setOutlineThickness(2.f);
  avatarMask_.setOutlineColor(sf::Color(0, 220, 255, 200));
  avatarMask_.setPosition(sf::Vector2f(20.f - 2.f, 14.f - 2.f));

  // ── Board ─────────────────────────────────────────────────────────────────
  boardBg_.setSize(sf::Vector2f(Layout::BOARD_SIZE, Layout::BOARD_SIZE));
  boardBg_.setFillColor(sf::Color(10, 18, 35, 220));
  boardBg_.setOutlineThickness(1.f);
  boardBg_.setOutlineColor(sf::Color(0, 180, 255, 80));
  boardBg_.setPosition(sf::Vector2f(Layout::BOARD_X, Layout::BOARD_Y));

  // ── End-game buttons ──────────────────────────────────────────────────────
  rematchBtn_.setSize(sf::Vector2f(210.f, Layout::BTN_H));
  rematchBtn_.setFillColor(sf::Color(0, 160, 90, 210));
  rematchBtn_.setOutlineThickness(2.f);
  rematchBtn_.setOutlineColor(sf::Color(0, 255, 120));
  rematchBtn_.setPosition(sf::Vector2f(145.f, Layout::BTN_Y));

  rematchText_.setFont(font_);
  rematchText_.setCharacterSize(18);
  rematchText_.setFillColor(sf::Color::White);
  rematchText_.setString("PLAY AGAIN");

  quitBtn_.setSize(sf::Vector2f(210.f, Layout::BTN_H));
  quitBtn_.setFillColor(sf::Color(160, 20, 50, 210));
  quitBtn_.setOutlineThickness(2.f);
  quitBtn_.setOutlineColor(sf::Color(255, 40, 80));
  quitBtn_.setPosition(sf::Vector2f(440.f, Layout::BTN_Y));

  quitText_.setFont(font_);
  quitText_.setCharacterSize(18);
  quitText_.setFillColor(sf::Color::White);
  quitText_.setString("QUIT");

  // ── Result text (hidden until gameOver) ───────────────────────────────────
  resultText_.setFont(font_);
  resultText_.setCharacterSize(36);
  resultText_.setStyle(sf::Text::Bold);
  resultText_.setPosition(sf::Vector2f(0.f, 210.f));

  // ── Symbol assignment ─────────────────────────────────────────────────────
  mySymVal_ = (game_->getSymbol() == 'X' || game_->getSymbol() == 'x') ? 1 : 2;
  oppSymVal_ = (mySymVal_ == 1) ? 2 : 1;
  isMyTurn_ = (mySymVal_ == 1); // X always goes first

  // ── IPC ───────────────────────────────────────────────────────────────────
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
    std::cerr << "[TicTacToe] WARNING: IPC segment creation failed.\n";
  }
}

void PlayingState::onExit() {}

// ─── processEvent
// ─────────────────────────────────────────────────────────────

void PlayingState::processEvent(const sf::Event &event) {
  if (gameOver_) {
    if (const auto *me = event.getIf<sf::Event::MouseButtonPressed>()) {
      if (me->button == sf::Mouse::Button::Left) {
        sf::Vector2f mp(static_cast<float>(me->position.x),
                        static_cast<float>(me->position.y));
        if (rematchBtn_.getGlobalBounds().contains(mp)) {
          if (ipcData_ && sharedMemory_) {
            sharedMemory_->lock();
            ipcData_->rematchRequested = true;
            sharedMemory_->unlock();
          }
          game_->getWindow().close();
        } else if (quitBtn_.getGlobalBounds().contains(mp)) {
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

  if (const auto *me = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (me->button == sf::Mouse::Button::Left && isMyTurn_) {
      int mx = me->position.x, my = me->position.y;
      int bx = static_cast<int>(Layout::BOARD_X);
      int by = static_cast<int>(Layout::BOARD_Y);
      int bs = static_cast<int>(Layout::BOARD_SIZE);
      if (mx >= bx && mx < bx + bs && my >= by && my < by + bs) {
        int col = (mx - bx) / static_cast<int>(Layout::CELL_SIZE);
        int row = (my - by) / static_cast<int>(Layout::CELL_SIZE);
        int idx = row * 3 + col;
        if (idx >= 0 && idx < 9 && board_[idx] == 0) {
          board_[idx] = mySymVal_;
          isMyTurn_ = false;
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

// ─── update
// ───────────────────────────────────────────────────────────────────

void PlayingState::update(sf::Time dt) {
  animClock_ += dt.asSeconds();

  // ── Inbound move polling ───────────────────────────────────────────────────
  if (!gameOver_ && ipcData_ && sharedMemory_) {
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

  // ── Status text ───────────────────────────────────────────────────────────
  if (!gameOver_) {
    float alpha = 0.55f + 0.45f * std::sin(animClock_ * 3.14f);
    if (isMyTurn_) {
      statusText_.setString("> YOUR TURN [" +
                            std::string(1, game_->getSymbol()) + "]");
      uint8_t g = static_cast<uint8_t>(80 + 175 * alpha);
      statusText_.setFillColor(sf::Color(0, g, 80));
    } else {
      statusText_.setString("AWAITING OPPONENT...");
      uint8_t r = static_cast<uint8_t>(160 + 95 * alpha);
      statusText_.setFillColor(sf::Color(r, 60, 60));
    }
  }

  // ── Particles ─────────────────────────────────────────────────────────────
  for (auto &p : particles_) {
    p.pos += p.vel * dt.asSeconds();
    p.vel.y += 60.f * dt.asSeconds(); // gravity
    p.lifetime -= dt.asSeconds();
    float t = 1.f - p.lifetime / p.maxLife;
    p.color.a = static_cast<uint8_t>(255 * (1.f - t));
  }
  particles_.erase(
      std::remove_if(particles_.begin(), particles_.end(),
                     [](const Particle &p) { return p.lifetime <= 0.f; }),
      particles_.end());

  // ── Centre button labels ───────────────────────────────────────────────────
  if (gameOver_) {
    auto centre = [](sf::Text &txt, const sf::RectangleShape &btn) {
      sf::FloatRect tb = txt.getLocalBounds();
      sf::FloatRect bb = btn.getGlobalBounds();
      txt.setPosition(sf::Vector2f(
          bb.position.x + (bb.size.x - tb.size.x) / 2.f - tb.position.x,
          bb.position.y + (bb.size.y - tb.size.y) / 2.f - tb.position.y));
    };
    centre(rematchText_, rematchBtn_);
    centre(quitText_, quitBtn_);
  }
}

// ─── spawnParticles
// ───────────────────────────────────────────────────────────

void PlayingState::spawnParticles() {
  // Pick colour based on winner
  sf::Color baseColor = (winner_ == mySymVal_) ? sf::Color(0, 255, 180)
                        : (winner_ == 0)       ? sf::Color(180, 120, 255)
                                               : sf::Color(255, 60, 80);
  for (int i = 0; i < 150; ++i) {
    Particle p;
    p.pos = sf::Vector2f(Layout::WIN_W / 2.f + (rand() % 200 - 100),
                         180.f + rand() % 80);
    float ang = (rand() % 360) * 3.14159f / 180.f;
    float spd = 60.f + rand() % 200;
    p.vel = sf::Vector2f(std::cos(ang) * spd, std::sin(ang) * spd - 80.f);
    p.maxLife = p.lifetime = 0.8f + (rand() % 100) / 100.f * 1.2f;
    p.color = sf::Color(static_cast<uint8_t>(
                            std::min(255, (int)baseColor.r + rand() % 60 - 30)),
                        static_cast<uint8_t>(
                            std::min(255, (int)baseColor.g + rand() % 60 - 30)),
                        static_cast<uint8_t>(
                            std::min(255, (int)baseColor.b + rand() % 60 - 30)),
                        255);
    particles_.push_back(p);
  }
}

// ─── checkWinCondition
// ────────────────────────────────────────────────────────

void PlayingState::checkWinCondition() {
  if (gameOver_)
    return;
  int wins[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
                    {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
  for (auto &w : wins) {
    if (board_[w[0]] && board_[w[0]] == board_[w[1]] &&
        board_[w[1]] == board_[w[2]]) {
      gameOver_ = true;
      winner_ = board_[w[0]];
      break;
    }
  }
  if (!gameOver_) {
    bool full = true;
    for (int i = 0; i < 9; ++i)
      if (!board_[i]) {
        full = false;
        break;
      }
    if (full) {
      gameOver_ = true;
      winner_ = 0;
    }
  }

  if (gameOver_) {
    if (winner_ == mySymVal_) {
      resultText_.setString("YOU WIN!  FATALITY.");
      resultText_.setFillColor(sf::Color(0, 255, 180));
    } else if (winner_ == oppSymVal_) {
      resultText_.setString("YOU LOSE. SYSTEM OFFLINE.");
      resultText_.setFillColor(sf::Color(255, 50, 80));
    } else {
      resultText_.setString("DRAW.  STALEMATE LOGGED.");
      resultText_.setFillColor(sf::Color(180, 130, 255));
    }
    sf::FloatRect rb = resultText_.getLocalBounds();
    resultText_.setPosition(
        sf::Vector2f((Layout::WIN_W - rb.size.x) / 2.f - rb.position.x, 210.f));

    if (!particlesSpawned_) {
      spawnParticles();
      particlesSpawned_ = true;
    }

    if (ipcData_ && sharedMemory_) {
      sharedMemory_->lock();
      ipcData_->gameOver = true;
      ipcData_->winner = winner_;
      sharedMemory_->unlock();
    }
  }
}

// ─── drawGlowText
// ─────────────────────────────────────────────────────────────

void PlayingState::drawGlowText(sf::RenderTarget &target, sf::Text &text,
                                sf::Color gc, int passes) {
  sf::Color orig = text.getFillColor();
  for (int i = passes; i >= 1; --i) {
    float off = static_cast<float>(i) * 2.f;
    sf::Color c(gc.r, gc.g, gc.b, static_cast<uint8_t>(60 / i));
    text.setFillColor(c);
    for (int dx = -1; dx <= 1; ++dx)
      for (int dy = -1; dy <= 1; ++dy) {
        if (!dx && !dy)
          continue;
        text.move(sf::Vector2f(dx * off, dy * off));
        target.draw(text);
        text.move(sf::Vector2f(-dx * off, -dy * off));
      }
  }
  text.setFillColor(orig);
  target.draw(text);
}

// ─── render
// ───────────────────────────────────────────────────────────────────

void PlayingState::render(sf::RenderTarget &target) {
  // ── Header bar ────────────────────────────────────────────────────────────
  sf::RectangleShape hbar(sf::Vector2f(Layout::WIN_W, Layout::HEADER_H));
  hbar.setFillColor(sf::Color(8, 12, 28));
  hbar.setPosition(sf::Vector2f(0.f, 0.f));
  target.draw(hbar);

  // Separator line
  sf::RectangleShape sep(sf::Vector2f(Layout::WIN_W, 2.f));
  sep.setFillColor(sf::Color(0, 200, 255, 120));
  sep.setPosition(sf::Vector2f(0.f, Layout::HEADER_H));
  target.draw(sep);

  // Avatar
  if (avatarLoaded_ && avatarSpriteOpt_) {
    target.draw(*avatarSpriteOpt_);
  } else {
    // Draw initials circle as fallback
    sf::CircleShape circle(36.f);
    circle.setFillColor(sf::Color(30, 40, 80));
    circle.setOutlineThickness(2.f);
    circle.setOutlineColor(sf::Color(0, 200, 255));
    circle.setPosition(sf::Vector2f(18.f, 14.f));
    target.draw(circle);
    sf::Text init(font_);
    init.setCharacterSize(28);
    init.setFillColor(sf::Color(0, 200, 255));
    std::string opp = game_->getOpponent();
    init.setString(opp.empty() ? "?" : std::string(1, std::toupper(opp[0])));
    sf::FloatRect ib = init.getLocalBounds();
    init.setPosition(
        sf::Vector2f(18.f + 36.f - ib.size.x / 2.f - ib.position.x,
                     14.f + 36.f - ib.size.y / 2.f - ib.position.y));
    target.draw(init);
  }
  target.draw(avatarMask_);

  // Title texts
  drawGlowText(target, titleText_, sf::Color(0, 200, 255), 2);
  target.draw(vsLabel_);

  // My symbol badge
  sf::Text myBadge(font_);
  myBadge.setCharacterSize(16);
  myBadge.setFillColor(sf::Color(200, 200, 200));
  myBadge.setString("YOU: " + std::string(1, game_->getSymbol()));
  myBadge.setPosition(sf::Vector2f(110.f, 74.f));
  target.draw(myBadge);

  // ── Board background ──────────────────────────────────────────────────────
  target.draw(boardBg_);

  // Animated pulsing grid lines
  float pulse = 0.55f + 0.45f * std::sin(animClock_ * 2.f);
  uint8_t lineAlpha = static_cast<uint8_t>(100 + 100 * pulse);

  // Vertical
  for (int i = 1; i <= 2; ++i) {
    float x = Layout::BOARD_X + i * Layout::CELL_SIZE;
    sf::RectangleShape line(sf::Vector2f(2.f, Layout::BOARD_SIZE));
    line.setFillColor(sf::Color(0, 200, 255, lineAlpha));
    line.setPosition(sf::Vector2f(x, Layout::BOARD_Y));
    target.draw(line);
  }
  // Horizontal
  for (int i = 1; i <= 2; ++i) {
    float y = Layout::BOARD_Y + i * Layout::CELL_SIZE;
    sf::RectangleShape line(sf::Vector2f(Layout::BOARD_SIZE, 2.f));
    line.setFillColor(sf::Color(0, 200, 255, lineAlpha));
    line.setPosition(sf::Vector2f(Layout::BOARD_X, y));
    target.draw(line);
  }

  // Symbols
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      int idx = row * 3 + col;
      if (board_[idx] == 1)
        drawSymbol(
            target, static_cast<int>(Layout::BOARD_X + col * Layout::CELL_SIZE),
            static_cast<int>(Layout::BOARD_Y + row * Layout::CELL_SIZE), 'X');
      else if (board_[idx] == 2)
        drawSymbol(
            target, static_cast<int>(Layout::BOARD_X + col * Layout::CELL_SIZE),
            static_cast<int>(Layout::BOARD_Y + row * Layout::CELL_SIZE), 'O');
    }
  }

  // Status bar
  if (!gameOver_)
    target.draw(statusText_);

  // ── End-game overlay ──────────────────────────────────────────────────────
  if (gameOver_) {
    sf::RectangleShape overlay(sf::Vector2f(Layout::WIN_W, 610.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 155));
    target.draw(overlay);

    drawGlowText(target, resultText_, resultText_.getFillColor(), 4);

    target.draw(rematchBtn_);
    drawGlowText(target, rematchText_, sf::Color(0, 255, 120), 1);
    target.draw(quitBtn_);
    drawGlowText(target, quitText_, sf::Color(255, 60, 80), 1);

    // Particles
    sf::CircleShape dot(3.f);
    for (const auto &p : particles_) {
      dot.setFillColor(p.color);
      dot.setPosition(p.pos);
      target.draw(dot);
    }
  }
}

// ─── drawSymbol
// ───────────────────────────────────────────────────────────────

void PlayingState::drawSymbol(sf::RenderTarget &target, int cx, int cy,
                              char sym) {
  float cell = Layout::CELL_SIZE;
  sf::Color mainCol = (sym == 'X') ? sf::Color(255, 40, 140)  // neon pink
                                   : sf::Color(40, 255, 160); // neon green
  sf::Color glowCol = mainCol;
  glowCol.a = 55;

  float margin = cell * 0.18f;
  float x0 = cx + margin, y0 = cy + margin;
  float x1 = cx + cell - margin, y1 = cy + cell - margin;

  if (sym == 'X') {
    // Draw two crossing thick lines as rectangles
    float thick = 10.f;
    float diag = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    // Line 1: top-left to bottom-right
    sf::RectangleShape line1(sf::Vector2f(diag, thick));
    line1.setFillColor(mainCol);
    line1.setOrigin(sf::Vector2f(0.f, thick / 2.f));
    line1.setPosition(sf::Vector2f(x0, y0));
    float angle = std::atan2(y1 - y0, x1 - x0) * 180.f / 3.14159f;
    line1.setRotation(sf::degrees(angle));

    // Glow
    sf::RectangleShape glow1 = line1;
    glow1.setSize(sf::Vector2f(diag, thick + 8.f));
    glow1.setOrigin(sf::Vector2f(0.f, (thick + 8.f) / 2.f));
    glow1.setFillColor(glowCol);
    target.draw(glow1);
    target.draw(line1);

    // Line 2: top-right to bottom-left
    sf::RectangleShape line2(sf::Vector2f(diag, thick));
    line2.setFillColor(mainCol);
    line2.setOrigin(sf::Vector2f(0.f, thick / 2.f));
    line2.setPosition(sf::Vector2f(x1, y0));
    float angle2 = std::atan2(y1 - y0, x0 - x1) * 180.f / 3.14159f;
    line2.setRotation(sf::degrees(angle2));
    sf::RectangleShape glow2 = line2;
    glow2.setSize(sf::Vector2f(diag, thick + 8.f));
    glow2.setOrigin(sf::Vector2f(0.f, (thick + 8.f) / 2.f));
    glow2.setFillColor(glowCol);
    target.draw(glow2);
    target.draw(line2);

  } else {
    // Draw O as two concentric circles
    float r = (x1 - x0) / 2.f;
    float cx2 = cx + cell / 2.f, cy2 = cy + cell / 2.f;
    // Outer glow
    sf::CircleShape outer(r + 5.f);
    outer.setFillColor(sf::Color::Transparent);
    outer.setOutlineThickness(6.f);
    outer.setOutlineColor(glowCol);
    outer.setOrigin(sf::Vector2f(r + 5.f, r + 5.f));
    outer.setPosition(sf::Vector2f(cx2, cy2));
    target.draw(outer);
    // Main ring
    sf::CircleShape ring(r);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(10.f);
    ring.setOutlineColor(mainCol);
    ring.setOrigin(sf::Vector2f(r, r));
    ring.setPosition(sf::Vector2f(cx2, cy2));
    target.draw(ring);
  }
}
