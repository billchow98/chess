#include "bb.h"

#include <bit>
#include <string>

#include "common.h"

namespace tuna::bb {

Bitboard from_sq(Square sq) {
    return 1_u64 << sq;
}

Square top_sq(Bitboard bb) {
    assert(bb != Empty);
    auto sq = std::countr_zero(bb);
    return sq;
}

Square next_sq(Bitboard &bb) {
    assert(bb != Empty);
    auto sq = top_sq(bb);
    bb &= bb - 1;
    return sq;
}

i32 popcnt(Bitboard bb) {
    return std::popcount(bb);
}

char debug_char(Bitboard bb, Square sq) {
    return '0' + !!(bb & from_sq(sq));
}

std::string debug_str(Bitboard bb) {
    std::string s;
    for (Rank rk = rank::_8; rk >= rank::_1; rk--) {
        for (File fl = file::A; fl <= file::H; fl++) {
            auto sq = square::init(rk, fl);
            s += debug_char(bb, sq);
        }
        if (rk > rank::_1) {
            s += '\n';
        }
    }
    return s;
}

}  // namespace tuna::bb
