#include "movegen.h"

#include <algorithm>
#include <array>
#include <string>
#include <unordered_set>

#include "bb.h"
#include "board.h"
#include "cfg.h"
#include "common.h"
#include "logging.h"
#include "lookup.h"

namespace tuna::movegen {

const auto KNIGHT_DIRS = std::array{
    dir::NNE, dir::ENE, dir::ESE, dir::SSE,
    dir::SSW, dir::WSW, dir::WNW, dir::NNW,
};

const auto KING_DIRS = std::array{
    dir::N, dir::NE, dir::E, dir::SE, dir::S, dir::SW, dir::W, dir::NW,
};

MoveGenerator::MoveGenerator(const Board &board) : board_(board) {}

// Pseudo-legal moves. We do not check for legal moves here.
// It is up to the caller to decide when to call is_legal.
void MoveGenerator::generate(Type type) {
    update_boardinfo();
    moves_.clear();
    moves_.reserve(cfg::MOVE_VEC_RESERVE_CAP);
    auto to_mask = get_to_mask(type);
    if (board_.checkers_count() >= 2) {
        generate_piece(piece::King, bi_.king_bb, to_mask);
        return;
    }
    generate_pawn(type, to_mask);
    for (Piece pc = piece::Knight; pc <= piece::Queen; pc++) {
        auto pc_bb = board_.bb(pc, board_.turn());
        generate_piece(pc, pc_bb, to_mask);
    }
    if (type == Evasions) {
        to_mask = ~board_.color_bb(board_.turn());
    }
    generate_piece(piece::King, bi_.king_bb, to_mask);
    generate_castlings(type);
}

const std::vector<Move> &MoveGenerator::moves() const {
    return moves_;
}

bool MoveGenerator::has_legal_move(const Board &board) {
    MoveGenerator gen(board);
    gen.generate_all();
    filter_legal(gen);
    return !gen.moves().empty();
}

// Slower than Board::is_legal. Does not assume move is pseudo-legal first.
// Used for TT move checking of special cases
bool MoveGenerator::is_legal_move(const Board &board, Move move) {
    MoveGenerator gen(board);
    gen.generate_all();
    auto &ms = gen.moves();
    return std::find(ms.begin(), ms.end(), move) != ms.end();
}

u64 MoveGenerator::perft(Board &board, i32 depth, bool is_root) {
    if (depth == 0) {
        return 1;
    }
    u64 ttl = 0;
    MoveGenerator gen(board);
    gen.generate_all();
    filter_legal(gen);
    for (auto m : gen.moves()) {
        board.make_move(m);
        u64 sub_ttl = perft(board, depth - 1, false);
        ttl += sub_ttl;
        board.unmake_move();
        if (is_root) {
            io::println("{}: {}", move::to_str(m), sub_ttl);
        }
    }
    if (is_root) {
        io::println("");
        io::println("Nodes searched: {}\n", ttl);
    }
    return ttl;
}

// Test function for Board::is_pseudo_legal
u64 MoveGenerator::perft_pseudo_legal(Board &board, i32 depth, bool is_root) {
    if (depth == 0) {
        return 1;
    }
    u64 ttl = 0;
    MoveGenerator gen(board);
    gen.generate_all();
    check_pseudo_legal(gen);
    filter_legal(gen);
    for (auto m : gen.moves()) {
        board.make_move(m);
        u64 sub_ttl = perft_pseudo_legal(board, depth - 1, false);
        ttl += sub_ttl;
        board.unmake_move();
        if (is_root) {
            io::println("{}: {}", move::to_str(m), sub_ttl);
        }
    }
    if (is_root) {
        io::println("");
        io::println("Nodes searched: {}\n", ttl);
    }
    return ttl;
}

void MoveGenerator::update_boardinfo() {
    bi_.king_bb = board_.bb(piece::King, board_.turn());
    bi_.king_sq = board_.king_sq(board_.turn());
}

Bitboard MoveGenerator::get_to_mask(Type type) const {
    switch (type) {
    case Evasions:
        if (board_.checkers_count() >= 2) {
            return ~board_.color_bb(board_.turn());
        } else {
            return board_.evasion_mask();
        }
    case Captures:
        return board_.color_bb(!board_.turn());
    case Quiets:
        return ~board_.all();
    default:
        unreachable();
    }
}

void MoveGenerator::add(Square from, Square to, Piece promotion) {
    moves_.push_back(move::init(from, to, promotion));
}

Bitboard MoveGenerator::rank_8() const {
    return board_.turn() == color::White ? bb::RANK_8 : bb::RANK_1;
}

Bitboard MoveGenerator::single_pushes() const {
    return board_.single_pushes(board_.bb(piece::Pawn, board_.turn()));
}

void MoveGenerator::generate_single_pushes(Bitboard to_mask) {
    auto tos = single_pushes() & ~rank_8() & to_mask;
    while (tos != bb::Empty) {
        auto to = bb::next_sq(tos);
        add(square::sub(to, dir::N, board_.turn()), to);
    }
}

Bitboard MoveGenerator::double_pushes() const {
    return board_.double_pushes(board_.bb(piece::Pawn, board_.turn()));
}

void MoveGenerator::generate_double_pushes(Bitboard to_mask) {
    auto tos = double_pushes() & to_mask;
    while (tos != bb::Empty) {
        auto to = bb::next_sq(tos);
        add(square::sub(to, dir::NN, board_.turn()), to);
    }
}

Bitboard MoveGenerator::pawn_capture_to_mask(Type type,
                                             Bitboard to_mask) const {
    assert(type != Quiets);
    auto theirs = board_.color_bb(!board_.turn());
    auto mask = theirs & to_mask;
    // Do not mask away ep target square if it exists
    // We will check all ep moves later in is_legal()
    if (board_.ep() != file::None) {
        auto ep_rk = rank::rel(rank::_6, board_.turn());
        auto ep_fl = board_.ep();
        auto ep_sq = square::init(ep_rk, ep_fl);
        mask ^= bb::from_sq(ep_sq);
    }
    return mask;
}

Bitboard MoveGenerator::quiet_promotion_tos(Bitboard to_mask) const {
    return single_pushes() & rank_8() & to_mask;
}

void MoveGenerator::generate_quiet_queen_promotion(Type type,
                                                   Bitboard to_mask) {
    // Do not generate queen promotion in Quiets.
    // Avoids duplicate moves in Captures,
    if (type == Quiets) {
        return;
    }
    // We want to generate technically quiet queen promotions in captures
    // In order to do so, we have to use a Quiets to_mask
    // Evasions to_mask is left untouched
    if (type == Captures) {
        to_mask = get_to_mask(Quiets);
    }
    auto tos = quiet_promotion_tos(to_mask);
    while (tos != bb::Empty) {
        auto to = bb::next_sq(tos);
        add(square::sub(to, dir::N, board_.turn()), to, piece::Queen);
    }
}

void MoveGenerator::generate_quiet_underpromotions(Type type,
                                                   Bitboard to_mask) {
    if (type == Captures) {
        return;
    }
    auto tos = quiet_promotion_tos(to_mask);
    while (tos != bb::Empty) {
        auto to = bb::next_sq(tos);
        for (Piece pc = piece::Rook; pc >= piece::Knight; pc--) {
            add(square::sub(to, dir::N, board_.turn()), to, pc);
        }
    }
}

void MoveGenerator::generate_quiet_promotions(Type type, Bitboard to_mask) {
    generate_quiet_queen_promotion(type, to_mask);
    generate_quiet_underpromotions(type, to_mask);
}

Bitboard MoveGenerator::pawn_captures(Direction d) const {
    auto pawns = board_.bb(piece::Pawn, board_.turn());
    // Do not & with color_bb[!turn] here. to_mask may contain ep square
    return bb::shift(pawns, d, board_.turn());
}

void MoveGenerator::generate_normal_pawn_captures(Direction d,
                                                  Bitboard to_mask) {
    auto tos = pawn_captures(d) & ~rank_8() & to_mask;
    while (tos != bb::Empty) {
        auto to = bb::next_sq(tos);
        add(square::sub(to, d, board_.turn()), to);
    }
}

void MoveGenerator::generate_promotion_captures(Direction d, Bitboard to_mask) {
    auto tos = pawn_captures(d) & rank_8() & to_mask;
    while (tos != bb::Empty) {
        auto to = bb::next_sq(tos);
        for (Piece pc = piece::Queen; pc >= piece::Knight; pc--) {
            add(square::sub(to, d, board_.turn()), to, pc);
        }
    }
}

// All captures including promotions
void MoveGenerator::generate_pawn_captures(Type type, Bitboard to_mask) {
    if (type == Quiets) {
        return;
    }
    to_mask = pawn_capture_to_mask(type, to_mask);
    for (auto d : {dir::NW, dir::NE}) {
        generate_normal_pawn_captures(d, to_mask);
        generate_promotion_captures(d, to_mask);
    }
}

void MoveGenerator::generate_pawn(Type type, Bitboard to_mask) {
    generate_single_pushes(to_mask);
    generate_double_pushes(to_mask);
    generate_quiet_promotions(type, to_mask);
    generate_pawn_captures(type, to_mask);
}

void MoveGenerator::generate_piece(Piece pc, Bitboard froms, Bitboard to_mask) {
    assert(pc != piece::Pawn);
    while (froms != bb::Empty) {
        auto from = bb::next_sq(froms);
        auto tos = lookup::attacks(pc, from, board_.all()) & to_mask;
        while (tos != bb::Empty) {
            auto to = bb::next_sq(tos);
            add(from, to);
        }
    }
}

void MoveGenerator::generate_castlings(Type type) {
    if (type == Quiets) {
        for (Castling c = 0; c < 4; c++) {
            auto &ci = board::CASTLING_INFO[c];
            if (bi_.king_sq == ci.king_from) {
                add(ci.king_from, ci.king_to);
            }
        }
    }
}

void MoveGenerator::generate_all() {
    if (board_.in_check()) {
        generate(Evasions);
    } else {
        generate(Quiets);
        auto quiets = moves_;
        generate(Captures);
        moves_.insert(moves_.end(), quiets.begin(), quiets.end());
    }
}

void MoveGenerator::filter_legal(MoveGenerator &gen) {
    std::erase_if(gen.moves_,
                  [&gen](Move move) { return !gen.board_.is_legal(move); });
}

bool on_board(Square from, Direction d, i32 k = 1) {
    if (k > 7) {
        return false;
    }
    auto bb = bb::from_sq(from);
    for (auto i = 0; i < k; i++) {
        bb = bb::shift(bb, d);
        if (bb == bb::Empty) {
            return false;
        }
    }
    return true;
}

void check_pseudo_legal(std::unordered_set<Move> &moves, const Board &board,
                        Square from, Square to, Piece promotion) {
    auto m = move::init(from, to, promotion);
    if (moves.contains(m) != board.is_pseudo_legal(m)) {
        logging::debug("board.is_pseudo_legal({}): {}", move::to_str(m),
                       board.is_pseudo_legal(m));
        std::string s;
        for (auto m : moves) {
            s += move::to_str(m);
            s += ", ";
        }
        s = s.substr(0, s.length() - 2);  // Remove trailing ", "
        logging::debug("moves: {{\n\t{}\n}}", s);
        logging::debug("board:\n{}", board.debug_str());
        abort();
    }
}

void check_pseudo_legal(std::unordered_set<Move> &moves, const Board &board,
                        Square from, Square to) {
    check_pseudo_legal(moves, board, from, to, piece::None);
    // Possible promotion
    if (square::rank(from) == rank::_7 && square::rank(to) == rank::_8) {
        if (abs(square::file(to) - square::file(from)) <= 1) {
            for (Piece pc = piece::Knight; pc <= piece::Queen; pc++) {
                check_pseudo_legal(moves, board, from, to, pc);
            }
        }
    }
}

void MoveGenerator::check_pseudo_legal(MoveGenerator &gen) {
    std::unordered_set<Move> moves(gen.moves().begin(), gen.moves().end());
    for (Square i = square::A1; i <= square::H8; i++) {
        for (auto d : KNIGHT_DIRS) {
            if (on_board(i, d)) {
                auto j = i + d;
                movegen::check_pseudo_legal(moves, gen.board_, i, j);
            }
        }
        for (auto d : KING_DIRS) {
            auto k = 1;
            while (on_board(i, d, k)) {
                auto j = i + d * k++;
                movegen::check_pseudo_legal(moves, gen.board_, i, j);
            }
        }
    }
}

}  // namespace tuna::movegen
