#ifndef TUNA_LOOKUP_H
#define TUNA_LOOKUP_H

#include <cassert>

#include <array>

#include "bb.h"
#include "common.h"

namespace tuna::lookup {

extern std::array<Bitboard, 64> KNIGHT_ATTACKS;
extern std::array<Bitboard, 64> KING_ATTACKS;

Bitboard in_between(Square from, Square to);

Bitboard bishop_attacks(Bitboard occ, Square sq);

Bitboard rook_attacks(Bitboard occ, Square sq);

inline Bitboard attacks(Piece pc, Square sq, Bitboard occ = bb::Empty) {
    assert(pc != piece::Pawn);
    switch (pc) {
    case piece::Knight:
        return KNIGHT_ATTACKS[sq];
    case piece::Bishop:
        return bishop_attacks(occ, sq);
    case piece::Rook:
        return rook_attacks(occ, sq);
    case piece::Queen:
        return bishop_attacks(occ, sq) | rook_attacks(occ, sq);
    case piece::King:
        return KING_ATTACKS[sq];
    default:
        unreachable();
    }
}

void init();

}  // namespace tuna::lookup

#endif
