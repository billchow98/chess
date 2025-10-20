#ifndef TUNA_BOARD_H
#define TUNA_BOARD_H

#include <array>
#include <sstream>
#include <string>
#include <vector>

#include "common.h"

namespace tuna::board {

struct PositionCmd {
    enum { Startpos, Fen } type;
    std::string fen;
    std::vector<Move> moves;
};

struct UndoInfo {
    Move move;  // from, to, promotion
    Piece captured_piece;
    File ep;
    CastleFlags castle_flags;
    Ply halfmove_clock;
    Hash hash;  // We still need this for repetition detection
    Bitboard checkers;
    Bitboard pinned;
};

struct CastlingInfo {
    Square king_from;
    Square king_to;
    Square rook_from;
    Square rook_to;
};

struct MoveInfo {
    Square from;
    Square to;
    Piece promotion;
    Piece from_pc;
    Piece to_pc;
    Rank from_rk;
    Rank to_rk;
    File from_fl;
    File to_fl;
};

class Board {
public:
    Board();

    void setup(PositionCmd cmd);
    void setup_fen(std::string fen);

    std::string debug_str() const;

    Bitboard bb(Piece pc, Color cr) const;
    Bitboard all() const;
    Square king_sq(Color sd) const;

    void make_move(Move move);
    void make_null_move();
    void unmake_move();
    void unmake_null_move();

    Bitboard single_pushes(Bitboard pawns) const;
    Bitboard double_pushes(Bitboard pawns) const;
    Bitboard evasion_mask() const;

    bool is_pseudo_legal(Move move) const;
    bool is_legal(Move move) const;
    bool in_check() const;
    bool is_capture(Move move) const;
    bool is_draw() const;

    Bitboard color_bb(Color sd) const;
    Piece piece_on(Square sq) const;
    Color turn() const;
    File ep() const;
    Hash hash() const;
    i32 checkers_count() const;
    Score mg_material() const;  // material always from white's perspective
    Score eg_material() const;  // material always from white's perspective
    i32 game_phase() const;

private:
    void update_moveinfo(Move move) const;

    void create_undo(Move move);

    void clear_ep();
    void make_turn();
    void make_move_start(Move move);

    void flip_piece(Color sd, Piece pc, Square sq);
    void remove_piece(Color sd, Piece pc, Square sq);
    void add_piece(Color sd, Piece pc, Square sq);
    void move_piece(Color sd, Piece pc, Square from, Square to);

    void move_from_piece();
    void remove_to_piece();

    bool is_double_push() const;
    bool is_ep() const;
    void set_ep(File fl);
    void handle_eps();

    bool is_promotion() const;
    void handle_promotions();

    void remove_castle_flags(CastleFlags cfs);
    void handle_castle_flags();

    bool is_castle() const;
    void handle_castle_moves();
    void handle_castles();

    void flip_turn();

    Bitboard pawn_attacks_from(Square sq, Color sd) const;
    Bitboard attacks_from(Piece pc, Square sq) const;
    Bitboard attackers_to(Square sq) const;

    Bitboard bishop_likes() const;
    Bitboard rook_likes() const;

    void update_checkers();
    void update_pinned();
    void update_infos();

    bool is_undo_castle() const;
    void undo_castles();

    bool is_undo_promotion() const;
    void undo_promotions();

    bool is_undo_ep(Piece captured) const;
    void undo_eps(Piece captured);

    void add_to_piece(Piece captured);
    void undo_move_from_piece();

    void unmake_move_main(UndoInfo &undo);

    void restore_ep(File ep_fl);
    void restore_castle_flags(CastleFlags cfs);
    void unmake_move_end(UndoInfo &undo);

    Bitboard rank_8() const;
    Bitboard rank_2() const;

    Bitboard pawn_attacks(Bitboard pawns) const;

    bool is_pseudo_legal_attack() const;
    bool is_pseudo_legal_evasion() const;

    bool is_attacked(Square sq, Bitboard attackers_mask) const;
    bool king_to_is_attacked(Square ksq) const;

    bool castling_possible(Castling c) const;
    bool is_legal_castle(Castling c, const CastlingInfo &ci) const;
    bool is_legal_castle() const;

    bool is_pinned() const;
    bool is_on_line(Square s0, Square s1, Square s2) const;

    bool is_legal_ep(Square ksq) const;

    void setup_fen_pieces(std::istringstream &iss);
    void setup_fen_turn(std::istringstream &iss);
    void add_castle_flags(CastleFlags cfs);
    void setup_fen_castle_flags(std::istringstream &iss);
    void setup_fen_ep(std::istringstream &iss);
    void setup_fen_halfmove_clock(std::istringstream &iss);
    void setup_fen_fullmove_cnt(std::istringstream &iss);

    bool has_legal_move() const;
    bool is_fifty_move_draw() const;
    i32 repetition_count() const;
    bool is_repetition_draw() const;
    Piece color_on(Square sq) const;

    char debug_char(Square sq) const;

    std::array<Bitboard, piece::size> piece_bb_;
    std::array<Bitboard, color::size> color_bb_;
    std::array<Piece, 64> piece_on_;
    Color turn_;
    File ep_;
    CastleFlags castle_flags_;
    Ply halfmove_clock_;
    u16 fullmove_cnt_;
    Hash hash_;
    std::vector<UndoInfo> undos_;
    Bitboard checkers_;
    Bitboard pinned_;
    // material always from white's perspective
    Score mg_material_;
    Score eg_material_;
    i32 game_phase_;
    mutable MoveInfo mi_;  // tmp variable
};

const auto CASTLING_INFO = std::array<CastlingInfo, 4>{{
    {square::E1, square::G1, square::H1, square::F1},
    {square::E1, square::C1, square::A1, square::D1},
    {square::E8, square::G8, square::H8, square::F8},
    {square::E8, square::C8, square::A8, square::D8},
}};

}  // namespace tuna::board

#endif
