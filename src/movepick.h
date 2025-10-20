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
    bool operator<(const ScoredMove &rhs) const;

    Move move;
    MoveScore score;
};

class MovePicker {
public:
    class Iterator {
    public:
        Iterator(MovePicker &mp);

        Iterator(MovePicker &mp, bool is_end);

        Move operator*();

        Iterator &operator++();

        bool operator==(Iterator &rhs);

    private:
        MovePicker &mp_;
        Move move_;
    };

    MovePicker(Board &board, Type type, Move tt_move,
               std::array<Move, cfg::KILLERS_COUNT> &killers,
               ButterflyHistory &butterfly_hist);

    Iterator begin();

    Iterator end();

    void skip_quiet_moves();

private:
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

    Board &board_;
    MoveGenerator gen_;
    Move tt_move_;
    std::array<Move, cfg::KILLERS_COUNT> &killers_;
    std::array<Move, cfg::KILLERS_COUNT>::iterator cur_killer_;
    ButterflyHistory &butterfly_hist_;
    Stage stage_;
    std::vector<ScoredMove>::iterator cur_;
    std::vector<ScoredMove> moves_;
    bool skip_quiets_;
};

}  // namespace tuna::movepick

#endif
