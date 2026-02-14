#include "Tile.hpp"

namespace Core {

Tile::Tile() : value(0), merged(false) {}

Tile::Tile(int val) : value(val), merged(false) {}

int Tile::getValue() const { return value; }

bool Tile::isEmpty() const { return value == 0; }

bool Tile::hasMerged() const { return merged; }

void Tile::setMerged(bool m) { merged = m; }

void Tile::resetMerged() { merged = false; }
void Tile::setValue(int val) { value = val; }

} // namespace Core
