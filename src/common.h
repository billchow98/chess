#ifndef TUNA_COMMON_H
#define TUNA_COMMON_H

#include <cstdint>

#include <array>
#include <limits>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

[[noreturn]] inline void todo() {
    std::abort();
}

namespace tuna {

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f64 = double;

u64 operator""_u64(unsigned long long int x);

i16 operator""_i16(unsigned long long int x);

// noreturn to suppress compiler warnings
[[noreturn]] void unreachable(
    std::source_location loc = std::source_location::current());

using Color = bool;
using Rank = i8;
using File = i8;
using Direction = i8;
using Score = i16;
using HistoryScore = i16;
using Ply = u8;
using Piece = u8;
using Square = u8;
using Castling = u8;
using CastleFlags = u8;
using Bound = u8;
using UciOptionType = u8;
using Move = u16;
using Bitboard = u64;
using Hash = u64;
using ButterflyHistory =
    std::array<std::array<std::array<HistoryScore, 64>, 64>, 2>;

// U8_MAX - 1 to prevent infinite loop from u8 overflow in iterative deepening
const auto PLY_MAX = 254;
const auto HISTORY_MAX = std::numeric_limits<HistoryScore>::max();
const auto HISTORY_MIN = static_cast<HistoryScore>(-HISTORY_MAX);

namespace piece {

enum { Pawn, Knight, Bishop, Rook, Queen, King, None };

const auto size = 6;  // only for Board field

std::optional<Piece> from_char(char c);

char to_char(Piece pc);

}  // namespace piece

namespace color {

enum { White, Black };

const auto size = 2;  // only for Board field

std::string debug_str(Color cr);

}  // namespace color

namespace rank {

enum { _1, _2, _3, _4, _5, _6, _7, _8 };

Rank rel(Rank rk, Color sd);

char to_char(Rank rk);

}  // namespace rank

namespace file {

enum { A, B, C, D, E, F, G, H, None };

char to_char(File fl);

std::string debug_str(File fl);

}  // namespace file

namespace square {

// clang-format off
enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};
// clang-format on

Square init(Rank rk, File fl);

Rank rank(Square sq);

File file(Square sq);

Square sub(Square sq, Direction d, Color sd);

Square from_str(std::string_view str);

std::string to_str(Square sq);

}  // namespace square

namespace dir {

// clang-format off
enum {
    N = 8, E = 1, S = -N, W = -E,
    NE = N + E, SE = S + E, SW = S + W, NW = N + W,
    NNE = N + NE, ENE = E + NE, ESE = E + SE, SSE = S + SE,
    SSW = S + SW, WSW = W + SW, WNW = W + NW, NNW = N + NW,
    NN = N + N, EE = E + E, SS = S + S, WW = W + W,
};
// clang-format on

Direction flip(Direction d);

}  // namespace dir

namespace move {

const Move Null = 0;

Move init(Square from, Square to, Piece promotion = piece::None);

Square from(Move move);

Square to(Move move);

Piece promotion(Move move);

bool is_promotion(Move move);

Move from_str(std::string_view str);

std::string to_str(Move move);

}  // namespace move

namespace score {

const auto MATE = 20'000;
const auto DRAW = 0;
const auto MIN = -MATE;
const auto MAX = MATE;  // Must not be type's max or score >= beta won't work

Score side_score(Score score, Color cr);

i32 mate_distance(Score score);

bool is_mate(Score score);

Score mate(Ply ply);

std::string to_str(Score score);

}  // namespace score

namespace castling {

enum { WhiteKingside, WhiteQueenside, BlackKingside, BlackQueenside };

}

namespace castle_flags {

enum {
    None = 0,
    WhiteKingside = 1 << castling::WhiteKingside,
    WhiteQueenside = 1 << castling::WhiteQueenside,
    BlackKingside = 1 << castling::BlackKingside,
    BlackQueenside = 1 << castling::BlackQueenside,
    WhiteAll = WhiteKingside | WhiteQueenside,
    BlackAll = BlackKingside | BlackQueenside,
    All = WhiteAll | BlackAll,
};

CastleFlags from_castling(Castling c);

void add_str(std::string &s, std::string_view sv);

std::string debug_str(CastleFlags cfs);

}  // namespace castle_flags

}  // namespace tuna

#endif
