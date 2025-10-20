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

class MoveGenerator {
public:
    MoveGenerator(const Board &board);

    void generate(Type type);

    const std::vector<Move> &moves() const;

    static bool has_legal_move(const Board &board);
    static bool is_legal_move(const Board &board, Move move);

    static u64 perft(Board &board, i32 depth, bool is_root = true);
    static u64 perft_pseudo_legal(Board &board, i32 depth, bool is_root = true);

private:
    void update_boardinfo();

    Bitboard get_to_mask(Type type) const;

    void add(Square from, Square to, Piece promotion = piece::None);

    Bitboard rank_8() const;
    Bitboard single_pushes() const;
    void generate_single_pushes(Bitboard to_mask);

    Bitboard double_pushes() const;
    void generate_double_pushes(Bitboard to_mask);

    Bitboard pawn_capture_to_mask(Type type, Bitboard to_mask) const;

    Bitboard quiet_promotion_tos(Bitboard to_mask) const;

    void generate_quiet_queen_promotion(Type type, Bitboard to_mask);
    void generate_quiet_underpromotions(Type type, Bitboard to_mask);
    void generate_quiet_promotions(Type type, Bitboard to_mask);

    Bitboard pawn_captures(Direction d) const;
    void generate_normal_pawn_captures(Direction d, Bitboard to_mask);
    void generate_promotion_captures(Direction d, Bitboard to_mask);
    // All captures including promotions
    void generate_pawn_captures(Type type, Bitboard to_mask);

    void generate_pawn(Type type, Bitboard to_mask);
    void generate_piece(Piece pc, Bitboard froms, Bitboard to_mask);
    void generate_castlings(Type type);

    void generate_all();

    static void filter_legal(MoveGenerator &gen);
    static void check_pseudo_legal(MoveGenerator &gen);

    const Board &board_;
    std::vector<Move> moves_;
    BoardInfo bi_;  // tmp variable
};

// Convenience aliases
constexpr auto has_legal_move = &MoveGenerator::has_legal_move;

constexpr auto is_legal_move = &MoveGenerator::is_legal_move;

// Have to use functions to get default arguments working
inline u64 perft(Board &board, i32 depth, bool is_root = true) {
    return MoveGenerator::perft(board, depth, is_root);
}

inline u64 perft_pseudo_legal(Board &board, i32 depth, bool is_root = true) {
    return MoveGenerator::perft_pseudo_legal(board, depth, is_root);
}

}  // namespace tuna::movegen

#endif
