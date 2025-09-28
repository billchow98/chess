#ifndef TUNA_MOVEGEN_H
#define TUNA_MOVEGEN_H

#include "board.h"
#include "common.h"

namespace tuna::movegen {

using board::Board;

enum Type { Evasions, Captures, Quiets };

struct BoardInfo {
    Bitboard king_bb;
    Square king_sq;
};

struct MoveGenerator {
    Board &board;
    std::vector<Move> moves;

    MoveGenerator(Board &board);

    void update_boardinfo();

    Bitboard get_to_mask(Type type);

    void add(Square from, Square to, Piece promotion = piece::None);

    Bitboard rank_8();

    Bitboard single_pushes();

    void generate_single_pushes(Bitboard to_mask);

    Bitboard double_pushes();

    void generate_double_pushes(Bitboard to_mask);

    Bitboard pawn_capture_to_mask(Type type, Bitboard to_mask);

    Bitboard quiet_promotion_tos(Bitboard to_mask);

    void generate_quiet_queen_promotion(Type type, Bitboard to_mask);

    void generate_quiet_underpromotions(Type type, Bitboard to_mask);

    void generate_quiet_promotions(Type type, Bitboard to_mask);

    Bitboard pawn_captures(Direction d);

    void generate_normal_pawn_captures(Direction d, Bitboard to_mask);

    void generate_promotion_captures(Direction d, Bitboard to_mask);

    // All captures including promotions
    void generate_pawn_captures(Type type, Bitboard to_mask);

    void generate_pawn(Type type, Bitboard to_mask);

    void generate_piece(Piece pc, Bitboard froms, Bitboard to_mask);

    void generate_castlings(Type type);

    void generate(Type type);

    void generate_all();

protected:  // tmp variables
    BoardInfo bi;
};

bool has_legal_move(Board &board);

bool is_legal_move(Board &board, Move move);

u64 perft(Board &board, i32 depth, bool is_root = true);

u64 perft_pseudo_legal(Board &board, i32 depth, bool is_root = true);

}  // namespace tuna::movegen

#endif
