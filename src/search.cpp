#include "search.h"

#include <cassert>
#include <cmath>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "bb.h"
#include "cfg.h"
#include "common.h"
#include "eval.h"
#include "io.h"
#include "movepick.h"
#include "tt.h"

namespace tuna::search {

const auto MILLIS_MAX = std::numeric_limits<Millis>::max();

auto LMR_REDUCTION = std::array<std::array<Depth, 64>, 64>();

Searcher::Searcher(Board &board) : board(board) {}

void Searcher::new_game() {
    tt.clear();
    butterfly_hist = {};
}

void Searcher::resize_tt(u64 mb) {
    tt.resize(mb);
}

void Searcher::allocate_time(GoCmd &cmd) {
    max_millis = 0;
    auto time = board.turn == color::White ? cmd.wtime : cmd.btime;
    if (time) {
        max_millis += std::max(*time / 30_u64, 1_u64);
    }
    auto inc = board.turn == color::White ? cmd.winc : cmd.binc;
    if (inc) {
        max_millis += *inc;
    }
    if (!time && !inc) {
        max_millis = MILLIS_MAX;
    } else {
        max_millis = std::min(max_millis, *time);
    }
}

void Searcher::new_search(GoCmd &cmd) {
    clock_start = clock::now();
    stop_requested = false;
    max_depth = cmd.depth ? *cmd.depth : PLY_MAX;
    bestmove = move::Null;
    node_cnt = 0;
    depth_one_node_cnt = 0;
    root_score = 0;
    iter_depth = 0;
    cur_ply = 0;
    allocate_time(cmd);
    stk = {};
    stk.resize(cur_ply + 1);
}

void Searcher::reset_info() {
    stk.resize(cur_ply + 1);
}

Millis Searcher::elapsed() {
    auto clock_end = clock::now();
    Millis elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         clock_end - clock_start)
                         .count();
    return elapsed != 0 ? elapsed : 1;
}

bool Searcher::within_time_limit(Millis millis) {
    return millis + cfg::UCI_LATENCY_MS < max_millis;
}

void Searcher::check_limits_reached() {
    if (node_cnt % cfg::SEARCH_POLL_NODE_FREQ == 0 && iter_depth > 1) {
        if (!within_time_limit(elapsed())) {
            stop_requested = true;
        }
    }
}

void Searcher::make_move_end() {
    node_cnt++;
    check_limits_reached();
    cur_ply++;
    stk.resize(cur_ply + 1);
    stk[cur_ply].pv_line.clear();
}

void Searcher::make_move(Move move) {
    board.make_move(move);
    make_move_end();
}

void Searcher::unmake_move_end() {
    cur_ply--;
}

void Searcher::unmake_move() {
    board.unmake_move();
    unmake_move_end();
}

void Searcher::update_pv_line(Move move) {
    auto &pv_line = stk[cur_ply].pv_line;
    auto &child_pv = stk[cur_ply + 1].pv_line;
    pv_line.clear();
    pv_line.push_back(move);
    pv_line.insert(pv_line.end(), child_pv.begin(), child_pv.end());
}

Score Searcher::qsearch(Score alpha, Score beta) {
    using movepick::MovePicker;
    if (stop_requested) {
        return 0;
    }
    if (cur_ply == PLY_MAX) {
        return eval::evaluate(board);
    }
    if (board.is_draw()) {
        return score::DRAW;
    }
    // TT move here makes things worse
    MovePicker mp(board, movepick::Qsearch, move::Null, stk[cur_ply].killers,
                  butterfly_hist);
    auto best_score = score::MIN;
    if (!board.in_check()) {
        // Can only stand pat when not in check
        best_score = eval::evaluate(board);
    }
    if (best_score > alpha) {
        alpha = best_score;
        if (best_score >= beta) {
            return best_score;
        }
    }
    for (auto move : mp) {
        make_move(move);
        auto score = -qsearch(-beta, -alpha);
        unmake_move();
        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                alpha = score;
                if (score >= beta) {
                    break;
                }
                update_pv_line(move);
            }
        }
    }
    // Checkmate
    if (best_score == score::MIN) {
        return score::mate(cur_ply);
    }
    return best_score;
}

// Reverse futility pruning
bool Searcher::can_rfp(bool is_pv_node, Depth depth) {
    return !is_pv_node && depth <= 6 && !board.in_check();
}

Score Searcher::rfp_margin(Depth depth) {
    return 75 * depth;
}

bool Searcher::material_can_nmp() {
    // Current side has king and >= 2 piece. We have to skip NMP if
    // we have no more pieces (high chance of zugzwang)
    auto pc_cnt = bb::popcnt(board.color_bb[board.turn]);
    pc_cnt -= 1;                                              // King
    pc_cnt -= bb::popcnt(board.bb(piece::Pawn, board.turn));  // Pawns
    return pc_cnt >= 1;
}

// Null move pruning
bool Searcher::can_nmp(bool is_pv_node, Depth depth, Score eval, Score beta) {
    return !is_pv_node && depth >= 2 && !board.in_check() && eval >= beta &&
           material_can_nmp();
}

Depth Searcher::nmp_reduction(Depth depth) {
    return 2 + depth / 5;
}

void Searcher::make_null_move() {
    board.make_null_move();
    make_move_end();
}

void Searcher::unmake_null_move() {
    board.unmake_null_move();
    unmake_move_end();
}

// Late move pruning
bool Searcher::can_lmp(Depth depth, i32 moves_played) {
    return depth <= 2 && moves_played >= 4 + 6 * depth;
}

// Extensions (currently only check extension)
Depth Searcher::extension(bool gives_check) {
    return gives_check;
}

// Late move reductions
bool Searcher::can_lmr(Depth depth) {
    return depth >= 3;
}

Depth Searcher::lmr(Depth depth, i32 moves_played, bool is_pv_node) {
    depth = std::min(depth, 63_i16);
    moves_played = std::min(moves_played, 63);
    auto red = LMR_REDUCTION[depth][moves_played];
    red -= is_pv_node;
    return std::max(red, 0_i16);
}

// Killer heuristic
// Only unique killers are stored. FIFO replacement policy
void Searcher::update_killers(Move move) {
    auto &killers = stk[cur_ply].killers;
    auto it = std::find(killers.begin(), killers.end(), move);
    if (it == killers.end()) {
        // Remove last element and add move to the front of the 'queue'
        std::copy_backward(killers.begin(), killers.end() - 1, killers.end());
        killers[0] = move;
    } else {
        // Move existing match to the front of the 'queue'
        std::rotate(killers.begin(), it, it + 1);
    }
}

// Butterfly history heuristic
// i32 in case of overflow in extreme cases
i32 Searcher::butterfly_history_bonus(Depth depth) {
    return depth * depth;
}

// Need our own implementation to avoid stupid type mismatch issues
HistoryScore Searcher::clamp_history_score(i32 bonus) {
    if (bonus < HISTORY_MIN) {
        return HISTORY_MIN;
    }
    if (bonus > HISTORY_MAX) {
        return HISTORY_MAX;
    }
    return bonus;
}

HistoryScore &Searcher::get_butterfly_history(Move move) {
    auto sd = board.turn;
    auto from = move::from(move);
    auto to = move::to(move);
    return butterfly_hist[sd][from][to];
}

void Searcher::update_butterfly_history(Move move, HistoryScore clamped_bonus) {
    auto &hist = get_butterfly_history(move);
    hist += clamped_bonus - hist * abs(clamped_bonus) / HISTORY_MAX;
}

void Searcher::update_butterfly_history(Move move,
                                        std::vector<Move> &quiets_played,
                                        Depth depth) {
    auto bonus = butterfly_history_bonus(depth);
    auto clamped_bonus = clamp_history_score(bonus);
    update_butterfly_history(move, clamped_bonus);
    auto malus = -4 * bonus;
    auto clamped_malus = clamp_history_score(malus);
    for (auto move : quiets_played) {
        update_butterfly_history(move, clamped_malus);
    }
}

void Searcher::update_quiet_histories(Move move,
                                      std::vector<Move> &quiets_played,
                                      Depth depth) {
    update_killers(move);
    update_butterfly_history(move, quiets_played, depth);
}

// Principal variation search
Score Searcher::pvs(Depth depth, Score alpha, Score beta) {
    using movepick::MovePicker;
    if (stop_requested) {
        return 0;
    }
    if (cur_ply == PLY_MAX) {
        return eval::evaluate(board);
    }
    if (depth <= 0) {
        return qsearch(alpha, beta);
    }
    if (board.is_draw()) {
        return score::DRAW;
    }
    auto is_root_node = cur_ply == 0;
    auto is_pv_node = beta - alpha > 1;
    tt::Entry &tte = tt.find(board.hash);
    auto ttm = move::Null;
    if (tte.is_valid()) {
        ttm = tte.move;
        auto tts = tte.search_score(cur_ply);
        // Only using TT cutoffs when not PV does not seem to improve
        // strength. Worse by ~4 ELO after 2000 games (10+0.1)
        if (tte.depth >= depth) {
            if (tte.bound & tt::Lower && tts >= beta) {
                return tts;
            }
            if (tte.bound & tt::Upper && tts <= alpha) {
                return tts;
            }
        }
    }
    auto eval =
        tte.is_valid() ? tte.search_score(cur_ply) : eval::evaluate(board);
    if (can_rfp(is_pv_node, depth)) {
        auto margin = rfp_margin(depth);
        if (eval - margin >= beta) {
            return eval;
        }
    }
    if (can_nmp(is_pv_node, depth, eval, beta)) {
        Depth reduction = nmp_reduction(depth);
        make_null_move();
        Score score = -pvs(depth - reduction - 1, -beta, -beta + 1);
        unmake_null_move();
        if (score >= beta) {
            return score;
        }
    }
    MovePicker mp(board, movepick::Main, ttm, stk[cur_ply].killers,
                  butterfly_hist);
    auto best_score = score::MIN;
    auto best_move = move::Null;
    auto ttb = tt::Upper;
    auto in_check = board.in_check();
    std::vector<Move> quiets_played;
    // TODO: Find out why wrapping this in an `if (!in_check)` condition
    // changes the number of nodes searched
    quiets_played.reserve(cfg::MOVE_VEC_RESERVE_CAP);
    auto moves_played = 0;
    for (auto move : mp) {
        auto is_capture = board.is_capture(move);
        if (!is_root_node && can_lmp(depth, moves_played)) {
            mp.skip_quiet_moves();
        }
        make_move(move);
        auto is_first_move = moves_played == 0;
        auto gives_check = board.in_check();
        Score score;
        Depth ext = extension(gives_check);
        Depth new_depth = depth + ext - 1;
        if (is_first_move) {
            score = -pvs(new_depth, -beta, -alpha);
        } else {
            Depth red = 0;
            if (can_lmr(depth)) {
                red = lmr(depth, moves_played, is_pv_node);
            }
            score = -pvs(new_depth - red, -alpha - 1, -alpha);
            if (score > alpha && red > 0) {
                score = -pvs(new_depth, -alpha - 1, -alpha);
            }
            if (score > alpha && is_pv_node) {
                score = -pvs(new_depth, -beta, -alpha);
            }
        }
        unmake_move();
        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score > alpha) {
                alpha = score;
                ttb = tt::Exact;
                if (score >= beta) {
                    ttb = tt::Lower;
                    if (!in_check && !is_capture) {
                        update_quiet_histories(move, quiets_played, depth);
                    }
                    break;
                }
                update_pv_line(move);
            }
        }
        if (!in_check && !is_capture) {
            quiets_played.push_back(move);
        }
        moves_played++;
    }
    // Checkmate or stalemate
    if (best_score == score::MIN) {
        return board.in_check() ? score::mate(cur_ply) : score::DRAW;
    }
    tte.update(board.hash, best_move, best_score, depth, ttb, cur_ply);
    return best_score;
}

void Searcher::aspiration_window(Depth depth) {
    if (depth == 1) {
        root_score = pvs(depth, score::MIN, score::MAX);
        return;
    }
    auto delta = cfg::ASP_WINDOW_SIZE;
    auto alpha = root_score - delta;
    auto beta = root_score + delta;
    while (true) {
        root_score = pvs(depth, alpha, beta);
        if (root_score <= alpha) {
            alpha = std::max(root_score - delta, score::MIN);
        } else if (root_score >= beta) {
            beta = std::min(root_score + delta, score::MAX);
        } else {
            break;
        }
        delta *= 2;
    }
}

bool Searcher::can_search_next_depth() {
    // Not enough data to calculate branching factor
    if (iter_depth == 1) {
        depth_one_node_cnt = node_cnt;
        return true;
    }
    auto base = static_cast<f64>(node_cnt) / depth_one_node_cnt;
    auto exp = 1.0 / (iter_depth - 1);
    auto branching_factor = pow(base, exp);
    return within_time_limit(elapsed() * branching_factor);
}

std::string Searcher::pv_str() {
    std::string pv_str;
    auto &pv_line = stk[0].pv_line;
    bool first = true;
    for (auto move : pv_line) {
        if (!first) {
            pv_str += ' ';
        }
        pv_str += move::to_str(move);
        first = false;
    }
    return pv_str;
}

void Searcher::print_info() {
    assert(!stop_requested);
    auto millis = elapsed();
    auto nps = static_cast<u64>(1000. * node_cnt / millis);
    io::println(
        "info depth {} score {} nodes {} nps {} hashfull {} time {} pv {}",
        iter_depth, score::to_str(root_score), node_cnt, nps, tt.hashfull(),
        millis, pv_str());
}

void Searcher::update_bestmove() {
    assert(!stop_requested && !stk[0].pv_line.empty());
    bestmove = stk[0].pv_line[0];
}

void Searcher::iterative_deepening() {
    for (iter_depth = 1; iter_depth <= max_depth; iter_depth++) {
        reset_info();
        aspiration_window(iter_depth);
        if (stop_requested) {
            return;
        }
        print_info();
        update_bestmove();
        if (!can_search_next_depth()) {
            return;
        }
    }
}

void Searcher::print_bestmove() {
    assert(bestmove != move::Null);
    io::println("bestmove {}", move::to_str(bestmove));
}

// Must pass by value because we make a copy in std::thread
void Searcher::go(GoCmd cmd) {
    new_search(cmd);
    iterative_deepening();
    print_bestmove();
}

void Searcher::stop() {
    stop_requested = true;
}

void init() {
    for (auto depth = 1; depth < 64; depth++) {
        for (auto moves_played = 1; moves_played < 64; moves_played++) {
            LMR_REDUCTION[depth][moves_played] =
                log2(depth) * log2(moves_played) / 4.0;
        }
    }
}

}  // namespace tuna::search
