#include "lookup.h"

#if defined(__BMI2__)
#include <immintrin.h>
#endif

#include "common.h"

namespace tuna::lookup {

const auto KNIGHT_ATK_DIRS = std::array<Direction, 8>{
    dir::NNE, dir::ENE, dir::ESE, dir::SSE,
    dir::SSW, dir::WSW, dir::WNW, dir::NNW,
};

const auto KING_ATK_DIRS = std::array<Direction, 8>{
    dir::N, dir::NE, dir::E, dir::SE, dir::S, dir::SW, dir::W, dir::NW,
};

const auto BISHOP_ATK_DIRS = std::array<Direction, 4>{
    dir::NE,
    dir::SE,
    dir::SW,
    dir::NW,
};

const auto ROOK_ATK_DIRS = std::array<Direction, 4>{
    dir::N,
    dir::E,
    dir::S,
    dir::W,
};

auto IN_BETWEEN = std::array<std::array<Bitboard, 64>, 64>();
// No auto for successful compilation in GitHub Actions
std::array<Bitboard, 64> KNIGHT_ATTACKS;
std::array<Bitboard, 64> KING_ATTACKS;
auto PEXT_ATTACKS_TABLE = std::array<Bitboard, 107'648>();
auto ROOK_OFFSET = std::array<u32, 64>();
auto ROOK_MASK = std::array<Bitboard, 64>();
auto BISHOP_OFFSET = std::array<u32, 64>();
auto BISHOP_MASK = std::array<Bitboard, 64>();
auto next_pext_index = 0;

void init_in_between() {
    auto fill_in_between = [](Square i, Square j, Direction d) {
        for (auto sq = i + d; sq < j; sq += d) {
            IN_BETWEEN[i][j] ^= bb::from_sq(sq);
            IN_BETWEEN[j][i] ^= bb::from_sq(sq);
        }
    };
    for (Square i = square::A1; i <= square::H8; i++) {
        for (Square j = i + 1; j <= square::H8; j++) {
            auto i_rk = square::rank(i);
            auto i_fl = square::file(i);
            auto j_rk = square::rank(j);
            auto j_fl = square::file(j);
            if (i_rk == j_rk) {
                fill_in_between(i, j, dir::E);
            } else if (i_fl == j_fl) {
                fill_in_between(i, j, dir::N);
            } else if (i_rk + i_fl == j_rk + j_fl) {
                fill_in_between(i, j, dir::NW);
            } else if (i_rk - i_fl == j_rk - j_fl) {
                fill_in_between(i, j, dir::NE);
            }
        }
    }
}

void init_lookup_attacks(std::array<Bitboard, 64> &atks,
                         const std::array<Direction, 8> &dirs) {
    for (Square i = square::A1; i <= square::H8; i++) {
        auto from = bb::from_sq(i);
        for (auto d : dirs) {
            atks[i] ^= bb::shift(from, d);
        }
    }
}

void init_knight_attacks() {
    init_lookup_attacks(KNIGHT_ATTACKS, KNIGHT_ATK_DIRS);
}

void init_king_attacks() {
    init_lookup_attacks(KING_ATTACKS, KING_ATK_DIRS);
}

Bitboard mask_bb(Piece pc, Square sq) {
    auto &attack_dirs = pc == piece::Bishop ? BISHOP_ATK_DIRS : ROOK_ATK_DIRS;
    auto mask = bb::Empty;
    for (auto d : attack_dirs) {
        auto sq_bb = bb::from_sq(sq);
        sq_bb = bb::shift(sq_bb, d);
        while (bb::shift(sq_bb, d) != bb::Empty) {
            mask ^= sq_bb;
            sq_bb = bb::shift(sq_bb, d);
        }
    }
    return mask;
}

// Simple implementation: https://www.felixcloutier.com/x86/pdep
Bitboard pdep(u64 src, Bitboard mask) {
    auto dst = bb::Empty;
    auto j = 0;
    for (auto i = 0; i < 64; i++) {
        if (mask & 1_u64 << i) {
            dst ^= (src >> j++ & 1_u64) << i;
        }
    }
    return dst;
}

Bitboard attacks_bb(Piece pc, Square sq, Bitboard occ) {
    auto &attack_dirs = pc == piece::Bishop ? BISHOP_ATK_DIRS : ROOK_ATK_DIRS;
    auto attacks = bb::Empty;
    for (auto d : attack_dirs) {
        auto sq_bb = bb::from_sq(sq);
        sq_bb = bb::shift(sq_bb, d);
        while (sq_bb != bb::Empty) {
            attacks ^= sq_bb;
            if ((sq_bb & occ) != bb::Empty) {
                break;
            }
            sq_bb = bb::shift(sq_bb, d);
        }
    }
    return attacks;
}

void init_pext_attacks(Piece pc, std::array<u32, 64> &offset,
                       std::array<Bitboard, 64> &mask) {
    for (Square i = square::A1; i <= square::H8; i++) {
        offset[i] = next_pext_index;
        mask[i] = mask_bb(pc, i);
        auto mask_bits = bb::popcnt(mask[i]);
        for (auto j = 0; j < 1 << mask_bits; j++) {
            auto occ = pdep(j, mask[i]);
            PEXT_ATTACKS_TABLE[offset[i] + j] = attacks_bb(pc, i, occ);
        }
        next_pext_index += 1 << mask_bits;
    }
}

void init_bishop_attacks() {
    init_pext_attacks(piece::Bishop, BISHOP_OFFSET, BISHOP_MASK);
}

void init_rook_attacks() {
    init_pext_attacks(piece::Rook, ROOK_OFFSET, ROOK_MASK);
}

void init_attacks() {
    init_knight_attacks();
    init_king_attacks();
    init_bishop_attacks();
    init_rook_attacks();
}

void init() {
    init_in_between();
    init_attacks();
}

Bitboard in_between(Square from, Square to) {
    return IN_BETWEEN[from][to];
}

u64 pext(Bitboard src, Bitboard mask) {
#if defined(__BMI2__)
    return _pext_u64(src, mask);
#else
    // Fallback implementation
    auto dst = 0_u64;
    auto k = 1_u64;
    while (mask != bb::Empty) {
        auto lsb = mask & -mask;
        dst |= !!(src & lsb) * k;
        mask ^= lsb;
        k <<= 1;
    }
    return dst;
#endif
}

Bitboard pext_attacks(std::array<u32, 64> &offset,
                      std::array<Bitboard, 64> &mask, Bitboard occ, Square sq) {
    auto index = offset[sq] + pext(occ, mask[sq]);
    return PEXT_ATTACKS_TABLE[index];
}

Bitboard bishop_attacks(Bitboard occ, Square sq) {
    return pext_attacks(BISHOP_OFFSET, BISHOP_MASK, occ, sq);
}

Bitboard rook_attacks(Bitboard occ, Square sq) {
    return pext_attacks(ROOK_OFFSET, ROOK_MASK, occ, sq);
}

}  // namespace tuna::lookup
