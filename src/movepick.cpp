#include "movepick.h"

#include <cassert>

#include <algorithm>
#include <array>

#include "common.h"

namespace tuna::movepick {

bool ScoredMove::operator<(const ScoredMove &rhs) const {
    return score > rhs.score;
}

MovePicker::Iterator::Iterator(MovePicker &mp) : mp(mp), move(mp.next()) {}
MovePicker::Iterator::Iterator(MovePicker &mp, bool is_end)
    : mp(mp), move(move::Null) {}

Move MovePicker::Iterator::operator*() {
    return move;
}

MovePicker::Iterator &MovePicker::Iterator::operator++() {
    move = mp.next();
    return *this;
}

bool MovePicker::Iterator::operator==(MovePicker::Iterator &rhs) {
    return &mp == &rhs.mp && move == rhs.move;
}

MovePicker::MovePicker(Board &board, Type type, Move tt_move,
                       std::array<Move, cfg::KILLERS_COUNT> &killers,
                       ButterflyHistory &butterfly_hist)
    : board(board), gen(board), tt_move(tt_move), killers(killers),
      butterfly_hist(butterfly_hist), skip_quiets(false) {
    using namespace stage;
    if (board.in_check()) {
        stage = EvasionsTt;
    } else {
        stage = type == Type::Main ? MainTt : QsearchTt;
    }
}

MoveScore MovePicker::mvv_lva(Move move) {
    auto lva = board.piece_on[move::from(move)];
    auto mvv = board.piece_on[move::to(move)];
    auto promotion = move::promotion(move);
    if (promotion != piece::None) {
        // If not a promotion capture, must be a queen promotion
        assert(mvv != piece::None || promotion == piece::Queen);
        if (mvv == piece::None) {
            // piece::None != 0. Set to 0 as there is no piece.
            mvv = 0;
        }
        mvv += promotion - piece::Pawn;
    } else if (mvv == piece::None) {
        // This should be an ep move
        assert(lva == piece::Pawn);
        mvv = piece::Pawn;
    }
    return 6 * mvv - lva;
}

MoveScore MovePicker::history_score(Move move) {
    auto sd = board.turn;
    auto from = move::from(move);
    auto to = move::to(move);
    return butterfly_hist[sd][from][to];
}

MoveScore MovePicker::score_move(movegen::Type type, Move move) {
    switch (type) {
    case movegen::Evasions:
        return board.is_capture(move) ? mvv_lva(move) + 2 * HISTORY_MAX
                                      : history_score(move);
    case movegen::Captures:
        return mvv_lva(move);
    case movegen::Quiets:
        return history_score(move);
    default:
        unreachable();
    }
}

void MovePicker::sort_moves(movegen::Type type) {
    moves.resize(gen.moves.size());
    for (auto i = 0; i < gen.moves.size(); i++) {
        moves[i] = {gen.moves[i], score_move(type, gen.moves[i])};
    }
    std::sort(moves.begin(), moves.end());
}

void MovePicker::init_killers() {
    cur_killer = killers.begin();
    stage++;
}

void MovePicker::generate(movegen::Type type) {
    gen.generate(type);
    sort_moves(type);
    cur = moves.begin();
    stage++;
}

bool MovePicker::is_fully_legal(Move move) {
    return move != move::Null && board.is_pseudo_legal(move) &&
           board.is_legal(move);
}

Move MovePicker::next_killer() {
    while (cur_killer != killers.end()) {
        auto move = *cur_killer++;
        if (move != tt_move && is_fully_legal(move)) {
            return move;
        }
    }
    return move::Null;
}

bool MovePicker::is_repeated_move(Move move) {
    return move == tt_move ||
           std::find(killers.begin(), killers.end(), move) != killers.end();
}

Move MovePicker::retrieve_next() {
    while (cur != moves.end()) {
        auto move = cur++->move;
        if (!is_repeated_move(move) && board.is_legal(move)) {
            return move;
        }
    }
    return move::Null;
}

Move MovePicker::next() {
    Move move;
    switch (stage) {
        using namespace stage;
    case EvasionsTt:
    case MainTt:
    case QsearchTt:
        stage++;
        if (is_fully_legal(tt_move)) {
            return tt_move;
        } else {
            return next();
        }
    case EvasionsInit:
        generate(movegen::Evasions);
        return next();
    case MainCapturesInit:
    case QsearchCapturesInit:
        generate(movegen::Captures);
        return next();
    case MainKillersInit:
        init_killers();
        return next();
    case MainQuietsInit:
        if (!skip_quiets) {
            generate(movegen::Quiets);
        } else {
            stage++;
        }
        return next();
    case MainKillers:
        if ((move = next_killer()) != move::Null) {
            return move;
        } else {
            stage++;
            return next();
        }
    case Evasions:
    case MainCaptures:
    case QsearchCaptures:
    case MainQuiets:
        if (!skip_quiets && (move = retrieve_next()) != move::Null) {
            return move;
        } else {
            stage++;
            return next();
        }
    case EvasionsEnd:
    case MainEnd:
    case QsearchEnd:
        return move::Null;
    default:
        unreachable();
    }
}

MovePicker::Iterator MovePicker::begin() {
    return {*this};
}

MovePicker::Iterator MovePicker::end() {
    return {*this, true};
}

void MovePicker::skip_quiet_moves() {
    skip_quiets = true;
}

}  // namespace tuna::movepick
