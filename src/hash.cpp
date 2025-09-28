#include "hash.h"

#include <array>
#include <random>

#include "common.h"

namespace tuna::hash {

// No auto for successful compilation in GitHub Actions
std::array<std::array<std::array<Hash, 64>, 6>, 2> piece;
Hash side;
std::array<Hash, 1 << 4> castling;
std::array<Hash, 8> ep;

auto rng = std::mt19937_64();

void init() {
    for (auto i = 0; i < 2; i++) {
        for (auto j = 0; j < 6; j++) {
            for (auto k = 0; k < 64; k++) {
                piece[i][j][k] = rng();
            }
        }
    }
    side = rng();
    for (auto &x : castling) {
        x = rng();
    }
    for (auto &x : ep) {
        x = rng();
    }
}

}  // namespace tuna::hash
