#include "common.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <array>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

namespace tuna {

u64 operator""_u64(unsigned long long int x) {
    return x;
}

i16 operator""_i16(unsigned long long int x) {
    return x;
}

void unreachable(std::source_location loc) {
    auto msg =
        std::format("Unreachable code reached: function {}, file {}, line {}.",
                    loc.function_name(), loc.file_name(), loc.line());
    fprintf(stderr, "%s\n", msg.c_str());
    abort();
}

namespace piece {

const auto PC_STR = "pnbrqk.";

std::optional<Piece> from_char(char c) {
    for (Piece pc = piece::Pawn; pc <= piece::King; pc++) {
        if (PC_STR[pc] == c) {
            return pc;
        }
    }
    return {};
}

char to_char(Piece pc) {
    assert(piece::Pawn <= pc && pc <= piece::None);
    assert(piece::None == piece::King + 1);
    return PC_STR[pc - piece::Pawn];
}

}  // namespace piece

namespace color {

std::string debug_str(Color cr) {
    return cr == White ? "color::White" : "color::Black";
}

}  // namespace color

namespace rank {

Rank rel(Rank rk, Color sd) {
    return sd == color::White ? rk : _8 - rk;
}

char to_char(Rank rk) {
    return static_cast<char>('1' + (rk - _1));
}

}  // namespace rank

namespace file {

char to_char(File fl) {
    return static_cast<char>('a' + (fl - A));
}

std::string debug_str(File fl) {
    if (fl == None) {
        return "File::None";
    }
    return std::format("File::{:c}", toupper(to_char(fl)));
}

}  // namespace file

namespace square {

Square init(Rank rk, File fl) {
    return rk * 8 + fl;
}

Rank rank(Square sq) {
    return sq / 8;
}

File file(Square sq) {
    return sq % 8;
}

Square sub(Square sq, Direction d, Color sd) {
    return sd == color::White ? sq - d : sq - dir::flip(d);
}

Square from_str(std::string_view str) {
    assert(str.length() == 2);
    return init(str[1] - '1', str[0] - 'a');
}

std::string to_str(Square sq) {
    return std::format("{}{}", file::to_char(file(sq)),
                       rank::to_char(rank(sq)));
}

}  // namespace square

namespace dir {

Direction flip(Direction d) {
    switch (d) {
    case N:
    case NN:
    case S:
    case SS:
        return -d;
    case NNE:
    case NE:
    case E:
    case SE:
    case SSE:
        return -(d - E) + E;
    case SSW:
    case SW:
    case W:
    case NW:
    case NNW:
        return -(d - W) + W;
    case ENE:
    case EE:
    case ESE:
        return -(d - EE) + EE;
    case WSW:
    case WW:
    case WNW:
        return -(d - WW) + WW;
    default:
        unreachable();
    }
}

}  // namespace dir

namespace move {

Move init(Square from, Square to, Piece promotion) {
    assert(promotion != piece::Pawn && promotion != piece::King);
    return from + (to << 6) + (promotion << 12);
}

Square from(Move move) {
    return move & ((1 << 6) - 1);
}

Square to(Move move) {
    return (move >> 6) & ((1 << 6) - 1);
}

Piece promotion(Move move) {
    return move >> 12;
}

bool is_promotion(Move move) {
    return promotion(move) != piece::None;
}

Move from_str(std::string_view str) {
    if (str.length() < 4 || str.length() > 5) {
        return move::Null;
    }
    auto from = square::from_str(str.substr(0, 2));
    auto to = square::from_str(str.substr(2, 2));
    if (str.length() == 4) {
        return init(from, to, piece::None);
    }
    auto promotion = piece::from_char(str[4]);
    if (!promotion || *promotion < piece::Knight || *promotion > piece::Queen) {
        return move::Null;
    }
    return init(from, to, *promotion);
}

std::string to_str(Move move) {
    auto from_str = square::to_str(from(move));
    auto to_str = square::to_str(to(move));
    auto promotion_str = std::string(1, piece::to_char(promotion(move)));
    promotion_str = promotion_str != "." ? promotion_str : "";
    return std::format("{}{}{}", from_str, to_str, promotion_str);
}

}  // namespace move

namespace score {

Score side_score(Score score, Color cr) {
    return cr == color::White ? score : -score;
}

i32 mate_distance(Score score) {
    return MATE - abs(score);
}

bool is_mate(Score score) {
    return mate_distance(score) <= PLY_MAX;
}

Score mate(Ply ply) {
    return -MATE + ply;
}

std::string to_str(Score score) {
    if (is_mate(score)) {
        // Possible raw scores: -MATE, MATE - 1, -(MATE - 2), MATE - 3, ...
        // Uci mate scores:         0,        1,          -1,        2, ...
        auto mate_score = score >= 0 ? (mate_distance(score) + 1) / 2
                                     : -mate_distance(score) / 2;
        return std::format("mate {}", mate_score);
    } else {
        return std::format("cp {}", score);
    }
}

}  // namespace score

namespace castle_flags {

const auto DEBUG_STR = std::array<std::string_view, 4>{
    "castle_flags::WhiteKingside",
    "castle_flags::WhiteQueenside",
    "castle_flags::BlackKingside",
    "castle_flags::BlackQueenside",
};

CastleFlags from_castling(Castling c) {
    return 1 << c;
}

void add_str(std::string &s, std::string_view sv) {
    if (!s.empty()) {
        s += " | ";
    }
    s += sv;
}

std::string debug_str(CastleFlags cfs) {
    std::string s;
    for (Castling c = 0; c < 4; c++) {
        if (cfs & (1 << c)) {
            add_str(s, DEBUG_STR[c]);
        }
    }
    return s;
}

}  // namespace castle_flags

}  // namespace tuna
