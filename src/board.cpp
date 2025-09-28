#include "board.h"

#include <cctype>

#include <algorithm>
#include <array>
#include <format>
#include <sstream>
#include <string>

#include "bb.h"
#include "common.h"
#include "eval.h"
#include "hash.h"
#include "lookup.h"
#include "movegen.h"

namespace tuna::board {

const auto STARTPOS_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const auto CASTLING_FEN = "KQkq";

Board::Board() {
    setup_fen(STARTPOS_FEN);
}

Bitboard Board::bb(Piece pc, Color cr) {
    return piece_bb[pc] & color_bb[cr];
}

Bitboard Board::all() {
    return color_bb[color::White] | color_bb[color::Black];
}

void Board::update_moveinfo(Move move) {
    mi.from = move::from(move);
    mi.to = move::to(move);
    mi.promotion = move::promotion(move);
    mi.from_pc = piece_on[mi.from];
    mi.to_pc = piece_on[mi.to];
    mi.from_rk = square::rank(mi.from);
    mi.to_rk = square::rank(mi.to);
    mi.from_fl = square::file(mi.from);
    mi.to_fl = square::file(mi.to);
}

void Board::create_undo(Move move) {
    UndoInfo undo;
    undo.move = move;
    // Corner case: en passant move.
    // But captured_piece == piece::None will not be an issue.
    undo.captured_piece = piece_on[move::to(move)];
    undo.ep = ep;
    undo.castle_flags = castle_flags;
    undo.halfmove_clock = halfmove_clock;
    undo.hash = hash;  // Needed for repetition detection for now
    undo.checkers = checkers;
    undo.pinned = pinned;
    undos.push_back(undo);
}

void Board::clear_ep() {
    if (ep != file::None) {
        hash ^= hash::ep[ep];
        ep = file::None;
    }
}

void Board::make_turn() {
    clear_ep();
    halfmove_clock++;
    if (turn == color::Black) {
        fullmove_cnt++;
    }
}

void Board::make_move_start(Move move) {
    create_undo(move);
    make_turn();
}

void Board::flip_piece(Color sd, Piece pc, Square sq) {
    auto delta_bb = bb::from_sq(sq);
    piece_bb[pc] ^= delta_bb;
    color_bb[sd] ^= delta_bb;
    hash ^= hash::piece[sd][pc][sq];
}

void Board::remove_piece(Color sd, Piece pc, Square sq) {
    flip_piece(sd, pc, sq);
    piece_on[sq] = piece::None;
    mg_material -= eval::mg_piece_value(sd, pc, sq);
    eg_material -= eval::eg_piece_value(sd, pc, sq);
    game_phase -= eval::piece_phase(pc);
}

void Board::add_piece(Color sd, Piece pc, Square sq) {
    flip_piece(sd, pc, sq);
    piece_on[sq] = pc;
    mg_material += eval::mg_piece_value(sd, pc, sq);
    eg_material += eval::eg_piece_value(sd, pc, sq);
    game_phase += eval::piece_phase(pc);
}

void Board::move_piece(Color sd, Piece pc, Square from, Square to) {
    remove_piece(sd, pc, from);
    add_piece(sd, pc, to);
}

void Board::move_from_piece() {
    move_piece(turn, mi.from_pc, mi.from, mi.to);
    if (mi.from_pc == piece::Pawn || mi.to_pc != piece::None) {
        halfmove_clock = 0;
    }
}

void Board::remove_to_piece() {
    if (mi.to_pc != piece::None) {
        remove_piece(!turn, mi.to_pc, mi.to);
    }
}

bool Board::is_double_push() {
    return mi.from_pc == piece::Pawn && abs(mi.to_rk - mi.from_rk) == 2;
}

bool Board::is_ep() {
    return mi.from_pc == piece::Pawn && mi.from_fl != mi.to_fl &&
           mi.to_pc == piece::None;
}

void Board::set_ep(File fl) {
    assert(ep == file::None);
    ep = fl;
    hash ^= hash::ep[fl];
}

// Handles board.ep (double push) and ep captures
void Board::handle_eps() {
    if (is_double_push()) {
        auto ep_fl = square::file(mi.from);
        set_ep(ep_fl);
    } else if (is_ep()) {
        auto ep_sq = mi.to ^ 8;
        remove_piece(!turn, piece::Pawn, ep_sq);
    }
}

bool Board::is_promotion() {
    return mi.promotion != piece::None;
}

// Handles promotion to_pc
void Board::handle_promotions() {
    if (is_promotion()) {
        remove_piece(turn, piece::Pawn, mi.to);
        add_piece(turn, mi.promotion, mi.to);
    }
}

void Board::remove_castle_flags(CastleFlags cfs) {
    hash ^= hash::castling[castle_flags];
    castle_flags &= ~cfs;
    hash ^= hash::castling[castle_flags];
}

void Board::handle_castle_flags() {
    if (mi.from_pc == piece::Rook) {
        for (Castling c = 0; c < 4; c++) {
            if (mi.from == CASTLING_INFO[c].rook_from) {
                remove_castle_flags(castle_flags::from_castling(c));
            }
        }
    } else if (mi.from_pc == piece::King) {
        namespace cf = castle_flags;
        auto cfs = turn == color::White ? cf::WhiteAll : cf::BlackAll;
        remove_castle_flags(cfs);
    }
    for (Castling c = 0; c < 4; c++) {
        if (mi.to == CASTLING_INFO[c].rook_from) {
            remove_castle_flags(castle_flags::from_castling(c));
        }
    }
}

bool Board::is_castle() {
    return mi.from_pc == piece::King && abs(mi.to_fl - mi.from_fl) == 2;
}

void Board::handle_castle_moves() {
    if (is_castle()) {
        for (Castling c = 0; c < 4; c++) {
            if (mi.to == CASTLING_INFO[c].king_to) {
                auto rook_from = CASTLING_INFO[c].rook_from;
                auto rook_to = CASTLING_INFO[c].rook_to;
                move_piece(turn, piece::Rook, rook_from, rook_to);
            }
        }
    }
}

// Handles castling moves (king/rook moved / opp rook captured) and
// castling moves (move rook)
void Board::handle_castles() {
    handle_castle_flags();
    handle_castle_moves();
}

void Board::flip_turn() {
    turn = !turn;
    hash ^= hash::side;
}

Square Board::king_sq(Color sd) {
    return bb::top_sq(bb(piece::King, sd));
}

// sd: color of attackers
Bitboard Board::pawn_attacks_from(Square sq, Color sd) {
    using namespace dir;
    auto sq_bb = bb::from_sq(sq);
    auto sh = [this](Bitboard bb, Direction d) {
        return bb::shift(bb, d, turn);
    };
    return color_bb[sd] & (sd == turn ? sh(sq_bb, SW) | sh(sq_bb, SE)
                                      : sh(sq_bb, NW) | sh(sq_bb, NE));
}

Bitboard Board::attacks_from(Piece pc, Square sq) {
    if (pc == piece::Pawn) {
        return pawn_attacks_from(sq, color::White) |
               pawn_attacks_from(sq, color::Black);
    } else {
        return lookup::attacks(pc, sq, all());
    }
}

Bitboard Board::attackers_to(Square sq) {
    auto all_atkrs = bb::Empty;
#pragma GCC unroll 6
    for (Piece pc = piece::Pawn; pc <= piece::King; pc++) {
        all_atkrs |= attacks_from(pc, sq) & piece_bb[pc];
    }
    all_atkrs &= all();
    return all_atkrs;
}

void Board::update_checkers() {
    checkers = attackers_to(king_sq(turn)) & color_bb[!turn];
}

Bitboard Board::bishop_likes() {
    return piece_bb[piece::Bishop] | piece_bb[piece::Queen];
}

Bitboard Board::rook_likes() {
    return piece_bb[piece::Rook] | piece_bb[piece::Queen];
}

void Board::update_pinned() {
    pinned = bb::Empty;
    auto ksq = king_sq(turn);
    auto xrays = lookup::attacks(piece::Bishop, ksq) & bishop_likes() |
                 lookup::attacks(piece::Rook, ksq) & rook_likes();
    xrays &= color_bb[!turn];
    auto occ = all() ^ xrays;
    while (xrays) {
        auto xray = bb::next_sq(xrays);
        auto bb = lookup::in_between(ksq, xray) & occ;
        if (bb::popcnt(bb) == 1) {
            pinned |= bb;  // might repeat same sq
        }
    }
}

void Board::update_infos() {
    update_checkers();
    update_pinned();
}

void Board::make_move(Move move) {
    update_moveinfo(move);
    make_move_start(move);
    remove_to_piece();
    move_from_piece();
    handle_eps();
    handle_promotions();
    handle_castles();
    flip_turn();
    update_infos();
}

void Board::make_null_move() {
    make_move_start(move::Null);
    flip_turn();
    update_infos();
}

bool Board::is_undo_castle() {
    return mi.to_pc == piece::King && abs(mi.to_fl - mi.from_fl) == 2;
}

void Board::undo_castles() {
    if (is_undo_castle()) {
        for (Castling c = 0; c < 4; c++) {
            if (mi.to == CASTLING_INFO[c].king_to) {
                auto rook_from = CASTLING_INFO[c].rook_from;
                auto rook_to = CASTLING_INFO[c].rook_to;
                move_piece(turn, piece::Rook, rook_to, rook_from);
            }
        }
    }
}

bool Board::is_undo_promotion() {
    return is_promotion();
}

void Board::undo_promotions() {
    if (is_undo_promotion()) {
        remove_piece(turn, mi.promotion, mi.to);
        add_piece(turn, piece::Pawn, mi.to);
        mi.to_pc = piece::Pawn;
    }
}

bool Board::is_undo_ep(Piece captured) {
    return mi.to_pc == piece::Pawn && mi.from_fl != mi.to_fl &&
           captured == piece::None;
}

void Board::undo_eps(Piece captured) {
    if (is_undo_ep(captured)) {
        auto ep_sq = mi.to ^ 8;
        add_piece(!turn, piece::Pawn, ep_sq);
    }
}

void Board::add_to_piece(Piece captured) {
    if (captured != piece::None) {
        add_piece(!turn, captured, mi.to);
    }
}

void Board::undo_move_from_piece() {
    move_piece(turn, mi.to_pc, mi.to, mi.from);
}

void Board::unmake_move_main(UndoInfo &undo) {
    update_moveinfo(undo.move);
    undo_castles();
    undo_promotions();
    undo_eps(undo.captured_piece);
    undo_move_from_piece();
    add_to_piece(undo.captured_piece);
}

void Board::restore_ep(File ep_fl) {
    clear_ep();
    if (ep_fl != file::None) {
        set_ep(ep_fl);
    }
}

void Board::restore_castle_flags(CastleFlags cfs) {
    remove_castle_flags(castle_flags::All);
    add_castle_flags(cfs);
}

void Board::unmake_move_end(UndoInfo &undo) {
    restore_ep(undo.ep);
    restore_castle_flags(undo.castle_flags);
    halfmove_clock = undo.halfmove_clock;
    if (turn == color::Black) {
        fullmove_cnt--;
    }
    assert(hash == undo.hash);
    checkers = undo.checkers;
    pinned = undo.pinned;
}

void Board::unmake_move() {
    auto &undo = undos.back();
    flip_turn();
    unmake_move_main(undo);
    unmake_move_end(undo);
    undos.pop_back();
}

void Board::unmake_null_move() {
    auto &undo = undos.back();
    flip_turn();
    restore_ep(undo.ep);
    halfmove_clock = undo.halfmove_clock;
    if (turn == color::Black) {
        fullmove_cnt--;
    }
    assert(hash == undo.hash);
    checkers = undo.checkers;
    pinned = undo.pinned;
    undos.pop_back();
}

Bitboard Board::rank_8() {
    return turn == color::White ? bb::RANK_8 : bb::RANK_1;
}

Bitboard Board::single_pushes(Bitboard pawns) {
    return bb::shift(pawns, dir::N, turn) & ~all();
}

Bitboard Board::rank_2() {
    return turn == color::White ? bb::RANK_2 : bb::RANK_7;
}

Bitboard Board::double_pushes(Bitboard pawns) {
    pawns &= rank_2();
    auto tos = bb::shift(pawns, dir::N, turn) & ~all();
    return bb::shift(tos, dir::N, turn) & ~all();
}

Bitboard Board::pawn_attacks(Bitboard pawns) {
    return bb::shift(pawns, dir::NW, turn) | bb::shift(pawns, dir::NE, turn);
}

bool Board::is_pseudo_legal_attack() {
    auto from_bb = bb::from_sq(mi.from);
    auto to_bb = bb::from_sq(mi.to);
    if (mi.from_pc == piece::Pawn) {
        if ((rank_8() & to_bb) != bb::Empty) {
            return false;
        }
        // There should not be any ep move at this point
        if ((single_pushes(from_bb) & to_bb) == bb::Empty &&
            (double_pushes(from_bb) & to_bb) == bb::Empty &&
            (pawn_attacks(from_bb) & color_bb[!turn] & to_bb) == bb::Empty) {
            return false;
        }
    } else {
        auto atks = lookup::attacks(mi.from_pc, mi.from, all());
        if ((atks & to_bb) == bb::Empty) {
            return false;
        }
    }
    return true;
}

Bitboard Board::evasion_mask() {
    auto checker = bb::top_sq(checkers);
    auto ksq = bb::top_sq(bb(piece::King, turn));
    return lookup::in_between(checker, ksq) | checkers;
}

bool Board::is_pseudo_legal_evasion() {
    // We will handle king evasions later in is_legal
    if (mi.from_pc == piece::King) {
        return true;
    }
    if (bb::popcnt(checkers) >= 2) {
        return false;
    }
    auto to_bb = bb::from_sq(mi.to);
    return (to_bb & evasion_mask()) != bb::Empty;
}

// Test if TT move is valid
// Assumes the move is one that can be generated by MoveGenerator
bool Board::is_pseudo_legal(Move move) {
    update_moveinfo(move);
    if (is_ep() || is_promotion() || is_castle()) {
        return movegen::is_legal_move(*this, move);
    }
    auto from_bb = bb::from_sq(mi.from);
    auto to_bb = bb::from_sq(mi.to);
    if ((color_bb[turn] & from_bb) == bb::Empty ||
        (color_bb[turn] & to_bb) != bb::Empty) {
        return false;
    }
    if (!is_pseudo_legal_attack()) {
        return false;
    }
    return checkers == bb::Empty || is_pseudo_legal_evasion();
}

bool Board::is_attacked(Square sq, Bitboard attackers_mask) {
    return (attackers_to(sq) & attackers_mask) != bb::Empty;
}

bool Board::king_to_is_attacked(Square ksq) {
    remove_piece(turn, piece::King, ksq);
    auto atkd = is_attacked(mi.to, color_bb[!turn]);
    add_piece(turn, piece::King, ksq);
    return atkd;
}

bool Board::castling_possible(Castling c) {
    return castle_flags & castle_flags::from_castling(c);
}

bool Board::is_legal_castle(Castling c, const CastlingInfo &ci) {
    if (!castling_possible(c)) {
        return false;
    }
    auto ib = lookup::in_between(ci.king_from, ci.rook_from);
    if ((ib & all()) != bb::Empty) {
        return false;
    }
    auto ib_sq = bb::top_sq(lookup::in_between(mi.from, mi.to));
    return !is_attacked(ib_sq, color_bb[!turn]);
}

bool Board::is_legal_castle() {
    for (Castling c = 0; c < 4; c++) {
        auto &ci = CASTLING_INFO[c];
        if (mi.from == ci.king_from && mi.to == ci.king_to) {
            return is_legal_castle(c, ci);
        }
    }
    return true;
}

bool Board::is_pinned() {
    return (pinned & bb::from_sq(mi.from)) != bb::Empty;
}

bool Board::is_on_line(Square s0, Square s1, Square s2) {
    auto x0 = square::file(s0);
    auto x1 = square::file(s1);
    auto x2 = square::file(s2);
    auto y0 = square::rank(s0);
    auto y1 = square::rank(s1);
    auto y2 = square::rank(s2);
    if (x0 == x1) {
        return x1 == x2;
    }
    if (y0 == y1) {
        return y1 == y2;
    }
    if (x0 + y0 == x1 + y1) {
        return x1 + y1 == x2 + y2;
    }
    if (x0 - y0 == x1 - y1) {
        return x1 - y1 == x2 - y2;
    }
    return false;
}

bool Board::is_legal_ep(Square ksq) {
    auto ep = mi.to ^ 8;
    move_piece(turn, piece::Pawn, mi.from, mi.to);
    remove_piece(!turn, piece::Pawn, ep);
    auto atkd = is_attacked(ksq, color_bb[!turn]);
    add_piece(!turn, piece::Pawn, ep);
    move_piece(turn, piece::Pawn, mi.to, mi.from);
    return !atkd;
}

bool Board::is_legal(Move move) {
    update_moveinfo(move);
    auto ksq = bb::top_sq(bb(piece::King, turn));
    if (mi.from == ksq) {
        if (king_to_is_attacked(ksq)) {
            return false;
        }
        if (!is_legal_castle()) {
            return false;
        }
        return true;
    }
    if (is_pinned() && !is_on_line(ksq, mi.from, mi.to)) {
        return false;
    }
    if (is_ep()) {
        return is_legal_ep(ksq);
    }
    return true;
}

void Board::setup_fen_pieces(std::istringstream &iss) {
    for (auto &pc_bb : piece_bb) {
        pc_bb = bb::Empty;
    }
    for (auto &cr_bb : color_bb) {
        cr_bb = bb::Empty;
    }
    for (Rank rk = rank::_8; rk >= rank::_1; rk--) {
        for (File fl = file::A; fl <= file::H; fl++) {
            char c;
            iss >> c;
            if (isdigit(c)) {
                fl += c - '1';
            } else {
                auto pc = piece::from_char(tolower(c));
                if (!pc) {
                    return;
                }
                auto sd = isupper(c) ? color::White : color::Black;
                add_piece(sd, *pc, square::init(rk, fl));
            }
        }
        if (rk > rank::_1) {
            char c;
            iss >> c;  // Consume '/' char
        }
    }
}

void Board::setup_fen_turn(std::istringstream &iss) {
    turn = color::White;
    std::string turn;
    iss >> turn;
    if (turn == "b") {
        flip_turn();
    }
}

void Board::add_castle_flags(CastleFlags cfs) {
    hash ^= hash::castling[castle_flags];
    castle_flags |= cfs;
    hash ^= hash::castling[castle_flags];
}

void Board::setup_fen_castle_flags(std::istringstream &iss) {
    std::string cfs_str;
    iss >> cfs_str;
    castle_flags = castle_flags::None;
    for (auto ch : cfs_str) {
        for (Castling c = 0; c < 4; c++) {
            if (ch == CASTLING_FEN[c]) {
                add_castle_flags(castle_flags::from_castling(c));
            }
        }
    }
}

void Board::setup_fen_ep(std::istringstream &iss) {
    std::string ep_str;
    iss >> ep_str;
    ep = file::None;
    if (ep_str != "-") {
        auto sq = square::from_str(ep_str);
        auto fl = square::file(sq);
        set_ep(fl);
    }
}

void Board::setup_fen_halfmove_clock(std::istringstream &iss) {
    i32 hmc;
    iss >> hmc;
    halfmove_clock = hmc;
}

void Board::setup_fen_fullmove_cnt(std::istringstream &iss) {
    i32 fmc;
    iss >> fmc;
    fullmove_cnt = fmc;
}

void Board::setup_fen(std::string fen) {
    std::istringstream iss(fen);
    std::fill(piece_on.begin(), piece_on.end(), piece::None);
    hash = hash::Empty;
    mg_material = 0;
    eg_material = 0;
    game_phase = 0;
    setup_fen_pieces(iss);
    setup_fen_turn(iss);
    setup_fen_castle_flags(iss);
    setup_fen_ep(iss);
    setup_fen_halfmove_clock(iss);
    setup_fen_fullmove_cnt(iss);
    undos.clear();
    update_infos();
}

void Board::setup(PositionCmd cmd) {
    if (cmd.type == PositionCmd::Startpos) {
        cmd.fen = STARTPOS_FEN;
    }
    setup_fen(cmd.fen);
    for (auto move : cmd.moves) {
        make_move(move);
    }
}

bool Board::in_check() {
    return checkers != bb::Empty;
}

bool Board::has_legal_move() {
    return movegen::has_legal_move(*this);
}

bool Board::is_capture(Move move) {
    update_moveinfo(move);
    // Captures + queen promotions (same as movegen captures)
    return mi.promotion == piece::Queen ||
           (all() & bb::from_sq(mi.to)) != bb::Empty || is_ep();
}

bool Board::is_fifty_move_draw() {
    return halfmove_clock >= 100 && (!in_check() || has_legal_move());
}

i32 Board::repetition_count() {
    auto reps = 0;
    auto n = undos.size();
    auto mx = std::min(n, static_cast<size_t>(halfmove_clock));
    for (auto i = 4; i <= mx; i += 2) {
        if (hash == undos[n - i].hash) {
            reps++;
        }
    }
    return reps;
}

bool Board::is_repetition_draw() {
    return repetition_count() >= 2;
}

// Does not detect stalemate
bool Board::is_draw() {
    return is_fifty_move_draw() || is_repetition_draw();
}

Piece Board::color_on(Square sq) {
    return color_bb[color::White] & bb::from_sq(sq) ? color::White
                                                    : color::Black;
}

char Board::debug_char(Square sq) {
    auto pc_char = piece::to_char(piece_on[sq]);
    return color_on(sq) == color::White ? toupper(pc_char) : pc_char;
}

std::string Board::debug_str() {
    std::string s;
    for (Rank rk = rank::_8; rk >= rank::_1; rk--) {
        for (File fl = file::A; fl <= file::H; fl++) {
            auto sq = square::init(rk, fl);
            s += debug_char(sq);
        }
        s += '\n';
    }
    s += std::format("turn: {}\n", color::debug_str(turn));
    s += std::format("ep: {}\n", file::debug_str(ep));
    s += std::format("castle_flags: {}\n",
                     castle_flags::debug_str(castle_flags));
    s += std::format("halfmove_clock: {}\n", halfmove_clock);
    s += std::format("fullmove_cnt: {}\n", fullmove_cnt);
    s += std::format("hash: 0x{:016x}\n", hash);
    s += std::format("undos: size() = {}\n", undos.size());
    s += std::format("checkers:\n{}\n", bb::debug_str(checkers));
    s += std::format("pinned:\n{}", bb::debug_str(pinned));
    return s;
}

}  // namespace tuna::board
