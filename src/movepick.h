#ifndef TUNA_MOVEPICK_H
#define TUNA_MOVEPICK_H

#include <array>
#include <vector>

#include "board.h"
#include "cfg.h"
#include "common.h"
#include "movegen.h"

namespace tuna::movepick {

using board::Board;
using movegen::MoveGenerator;

using Stage = u8;
using MoveScore = i32;

enum Type { Main, Qsearch };

namespace stage {

enum {
    EvasionsTt,
    EvasionsInit,
    Evasions,
    EvasionsEnd,

    MainTt,
    MainCapturesInit,
    MainCaptures,
    MainKillersInit,
    MainKillers,
    MainQuietsInit,
    MainQuiets,
    MainEnd,

    QsearchTt,
    QsearchCapturesInit,
    QsearchCaptures,
    QsearchEnd,
};

}

struct ScoredMove {
    Move move;
    MoveScore score;

    bool operator<(const ScoredMove &rhs) const;
};

struct MovePicker {
    Board &board;
    MoveGenerator gen;
    Move tt_move;
    std::array<Move, cfg::KILLERS_COUNT> &killers;
    std::array<Move, cfg::KILLERS_COUNT>::iterator cur_killer;
    ButterflyHistory &butterfly_hist;
    Stage stage;
    std::vector<ScoredMove>::iterator cur;
    std::vector<ScoredMove> moves;
    bool skip_quiets;

    struct Iterator {
        MovePicker &mp;
        Move move;

        Iterator(MovePicker &mp);

        Iterator(MovePicker &mp, bool is_end);

        Move operator*();

        Iterator &operator++();

        bool operator==(Iterator &rhs);
    };

    MovePicker(Board &board, Type type, Move tt_move,
               std::array<Move, cfg::KILLERS_COUNT> &killers,
               ButterflyHistory &butterfly_hist);

    MoveScore mvv_lva(Move move);

    MoveScore history_score(Move move);

    MoveScore score_move(movegen::Type type, Move move);

    void sort_moves(movegen::Type type);

    void init_killers();

    void generate(movegen::Type type);

    bool is_fully_legal(Move move);

    Move next_killer();

    bool is_repeated_move(Move move);

    Move retrieve_next();

    Move next();

    Iterator begin();

    Iterator end();

    void skip_quiet_moves();
};

}  // namespace tuna::movepick

#endif
