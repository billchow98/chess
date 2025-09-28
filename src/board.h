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

struct Board {
    std::array<Bitboard, piece::size> piece_bb;
    std::array<Bitboard, color::size> color_bb;
    std::array<Piece, 64> piece_on;
    Color turn;
    File ep;
    CastleFlags castle_flags;
    Ply halfmove_clock;
    u16 fullmove_cnt;
    Hash hash;
    std::vector<UndoInfo> undos;
    Bitboard checkers;
    Bitboard pinned;
    // material always from white's perspective
    Score mg_material;
    Score eg_material;
    i32 game_phase;

    Board();

    Bitboard bb(Piece pc, Color cr);

    Bitboard all();

    void update_moveinfo(Move move);

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

    bool is_double_push();

    bool is_ep();

    void set_ep(File fl);

    void handle_eps();

    bool is_promotion();

    void handle_promotions();

    void remove_castle_flags(CastleFlags cfs);

    void handle_castle_flags();

    bool is_castle();

    void handle_castle_moves();

    void handle_castles();

    void flip_turn();

    Square king_sq(Color sd);

    Bitboard pawn_attacks_from(Square sq, Color sd);

    Bitboard attacks_from(Piece pc, Square sq);

    Bitboard attackers_to(Square sq);

    void update_checkers();

    Bitboard bishop_likes();

    Bitboard rook_likes();

    void update_pinned();

    void update_infos();

    void make_move(Move move);

    void make_null_move();

    bool is_undo_castle();

    void undo_castles();

    bool is_undo_promotion();

    void undo_promotions();

    bool is_undo_ep(Piece captured);

    void undo_eps(Piece captured);

    void add_to_piece(Piece captured);

    void undo_move_from_piece();

    void unmake_move_main(UndoInfo &undo);

    void restore_ep(File ep_fl);

    void restore_castle_flags(CastleFlags cfs);

    void unmake_move_end(UndoInfo &undo);

    void unmake_move();

    void unmake_null_move();

    Bitboard rank_8();

    Bitboard single_pushes(Bitboard pawns);

    Bitboard rank_2();

    Bitboard double_pushes(Bitboard pawns);

    Bitboard pawn_attacks(Bitboard pawns);

    bool is_pseudo_legal_attack();

    Bitboard evasion_mask();

    bool is_pseudo_legal_evasion();

    bool is_pseudo_legal(Move move);

    bool is_attacked(Square sq, Bitboard attackers_mask);

    bool king_to_is_attacked(Square ksq);

    bool castling_possible(Castling c);

    bool is_legal_castle(Castling c, const CastlingInfo &ci);

    bool is_legal_castle();

    bool is_pinned();

    bool is_on_line(Square s0, Square s1, Square s2);

    bool is_legal_ep(Square ksq);

    bool is_legal(Move move);

    void setup_fen_pieces(std::istringstream &iss);

    void setup_fen_turn(std::istringstream &iss);

    void add_castle_flags(CastleFlags cfs);

    void setup_fen_castle_flags(std::istringstream &iss);

    void setup_fen_ep(std::istringstream &iss);

    void setup_fen_halfmove_clock(std::istringstream &iss);

    void setup_fen_fullmove_cnt(std::istringstream &iss);

    void setup_fen(std::string fen);

    void setup(PositionCmd cmd);

    bool in_check();

    bool has_legal_move();

    bool is_capture(Move move);

    bool is_fifty_move_draw();

    i32 repetition_count();

    bool is_repetition_draw();

    bool is_draw();

    Piece color_on(Square sq);

    char debug_char(Square sq);

    std::string debug_str();

protected:  // tmp variables
    MoveInfo mi;
};

const auto CASTLING_INFO = std::array<CastlingInfo, 4>{{
    {square::E1, square::G1, square::H1, square::F1},
    {square::E1, square::C1, square::A1, square::D1},
    {square::E8, square::G8, square::H8, square::F8},
    {square::E8, square::C8, square::A8, square::D8},
}};

}  // namespace tuna::board

#endif
