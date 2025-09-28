#ifndef TUNA_HASH_H
#define TUNA_HASH_H

#include <array>

#include "common.h"

namespace tuna::hash {

const auto Empty = static_cast<Hash>(0);

extern std::array<std::array<std::array<Hash, 64>, 6>, 2> piece;
extern Hash side;
extern std::array<Hash, 1 << 4> castling;
extern std::array<Hash, 8> ep;

void init();

}  // namespace tuna::hash

#endif
