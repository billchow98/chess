#include "movepick.h"

#include <cassert>

#include <algorithm>
#include <array>

#include "common.h"

namespace tuna::movepick {

bool ScoredMove::operator<(const ScoredMove &rhs) const {
    return score > rhs.score;
}

MovePicker::Iterator::Iterator(MovePicker &mp) : mp_(mp), move_(mp.next()) {}
MovePicker::Iterator::Iterator(MovePicker &mp, bool is_end)
    : mp_(mp), move_(move::Null) {}

Move MovePicker::Iterator::operator*() {
    return move_;
}

MovePicker::Iterator &MovePicker::Iterator::operator++() {
    move_ = mp_.next();
    return *this;
}

bool MovePicker::Iterator::operator==(MovePicker::Iterator &rhs) {
    return &mp_ == &rhs.mp_ && move_ == rhs.move_;
}

MovePicker::MovePicker(Board &board, Type type, Move tt_move,
                       std::array<Move, cfg::KILLERS_COUNT> &killers,
                       ButterflyHistory &butterfly_hist)
    : board_(board), gen_(board), tt_move_(tt_move), killers_(killers),
      butterfly_hist_(butterfly_hist), skip_quiets_(false) {
    using namespace stage;
    if (board.in_check()) {
        stage_ = EvasionsTt;
    } else {
        stage_ = type == Type::Main ? MainTt : QsearchTt;
    }
}

MovePicker::Iterator MovePicker::begin() {
    return {*this};
}

MovePicker::Iterator MovePicker::end() {
    return {*this, true};
}

void MovePicker::skip_quiet_moves() {
    skip_quiets_ = true;
}

MoveScore MovePicker::mvv_lva(Move move) {
    auto lva = board_.piece_on(move::from(move));
    auto mvv = board_.piece_on(move::to(move));
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
    auto sd = board_.turn();
    auto from = move::from(move);
    auto to = move::to(move);
    return butterfly_hist_[sd][from][to];
}

MoveScore MovePicker::score_move(movegen::Type type, Move move) {
    switch (type) {
    case movegen::Evasions:
        return board_.is_capture(move) ? mvv_lva(move) + 2 * HISTORY_MAX
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
    auto gen_moves = gen_.moves();
    moves_.resize(gen_moves.size());
    for (auto i = 0; i < gen_moves.size(); i++) {
        moves_[i] = {gen_moves[i], score_move(type, gen_moves[i])};
    }
    std::sort(moves_.begin(), moves_.end());
}

void MovePicker::init_killers() {
    cur_killer_ = killers_.begin();
    stage_++;
}

void MovePicker::generate(movegen::Type type) {
    gen_.generate(type);
    sort_moves(type);
    cur_ = moves_.begin();
    stage_++;
}

bool MovePicker::is_fully_legal(Move move) {
    return move != move::Null && board_.is_pseudo_legal(move) &&
           board_.is_legal(move);
}

Move MovePicker::next_killer() {
    while (cur_killer_ != killers_.end()) {
        auto move = *cur_killer_++;
        if (move != tt_move_ && is_fully_legal(move)) {
            return move;
        }
    }
    return move::Null;
}

bool MovePicker::is_repeated_move(Move move) {
    return move == tt_move_ ||
           std::find(killers_.begin(), killers_.end(), move) != killers_.end();
}

Move MovePicker::retrieve_next() {
    while (cur_ != moves_.end()) {
        auto move = cur_++->move;
        if (!is_repeated_move(move) && board_.is_legal(move)) {
            return move;
        }
    }
    return move::Null;
}

Move MovePicker::next() {
    Move move;
    switch (stage_) {
        using namespace stage;
    case EvasionsTt:
    case MainTt:
    case QsearchTt:
        stage_++;
        if (is_fully_legal(tt_move_)) {
            return tt_move_;
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
        if (!skip_quiets_) {
            generate(movegen::Quiets);
        } else {
            stage_++;
        }
        return next();
    case MainKillers:
        if ((move = next_killer()) != move::Null) {
            return move;
        } else {
            stage_++;
            return next();
        }
    case Evasions:
    case MainCaptures:
    case QsearchCaptures:
    case MainQuiets:
        if (!skip_quiets_ && (move = retrieve_next()) != move::Null) {
            return move;
        } else {
            stage_++;
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

}  // namespace tuna::movepick
