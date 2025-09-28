#ifndef TUNA_BB_H
#define TUNA_BB_H

#include <cassert>

#include <string>

#include "common.h"

namespace tuna::bb {

const auto Empty = Bitboard(0_u64);
const auto RANK_1 = Bitboard(0x00000000000000FF_u64);
const auto RANK_2 = Bitboard(0x000000000000FF00_u64);
const auto RANK_7 = Bitboard(0x00FF000000000000_u64);
const auto RANK_8 = Bitboard(0xFF00000000000000_u64);
const auto FILE_A = Bitboard(0x0101010101010101_u64);
const auto FILE_B = Bitboard(0x0202020202020202_u64);
const auto FILE_G = Bitboard(0x4040404040404040_u64);
const auto FILE_H = Bitboard(0x8080808080808080_u64);
const auto FILE_AB = FILE_A | FILE_B;
const auto FILE_GH = FILE_G | FILE_H;

Bitboard from_sq(Square sq);

Square top_sq(Bitboard bb);

Square next_sq(Bitboard &bb);

i32 popcnt(Bitboard bb);

inline __attribute__((always_inline)) Bitboard shift(Bitboard bb, Direction d,
                                                     Color sd = color::White) {
    auto sh = [](Bitboard bb, Direction d) {
        return d > 0 ? bb << d : bb >> -d;
    };
    if (sd == color::Black) {
        d = dir::flip(d);
    }
    switch (d) {
        using namespace dir;
    case N:
    case NN:
    case S:
    case SS:
        return sh(bb, d);
    case NNE:
    case NE:
    case E:
    case SE:
    case SSE:
        return sh(bb & ~FILE_H, d);
    case SSW:
    case SW:
    case W:
    case NW:
    case NNW:
        return sh(bb & ~FILE_A, d);
    case ENE:
    case EE:
    case ESE:
        return sh(bb & ~FILE_GH, d);
    case WSW:
    case WW:
    case WNW:
        return sh(bb & ~FILE_AB, d);
    default:
        unreachable();
    }
}

char debug_char(Bitboard bb, Square sq);

std::string debug_str(Bitboard bb);

}  // namespace tuna::bb

#endif
