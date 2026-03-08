#pragma once

#include "core/Game.h"
#include <SFML/Graphics.hpp>

class State {
public:
  virtual ~State() = default;

  virtual void onEnter() = 0;
  virtual void onExit() = 0;
  virtual void processEvent(const sf::Event &event) = 0;
  virtual void update(sf::Time dt) = 0;
  virtual void render(sf::RenderTarget &target) = 0;

protected:
  Game *game_;

  explicit State(Game *game) : game_(game) {}
};
