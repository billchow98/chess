#ifndef TUNA_EVAL_H
#define TUNA_EVAL_H

#include "board.h"
#include "common.h"

namespace tuna::eval {

using board::Board;

Score mg_piece_value(Color sd, Piece pc, Square sq);

Score eg_piece_value(Color sd, Piece pc, Square sq);

i32 piece_phase(Piece pc);

Score evaluate(Board &board);

}  // namespace tuna::eval

#endif
