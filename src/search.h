#ifndef TUNA_SEARCH_H
#define TUNA_SEARCH_H

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#include "board.h"
#include "cfg.h"
#include "common.h"
#include "tt.h"

namespace tuna::search {

using board::Board;
using tt::Tt;

using Millis = u32;
using Depth = i16;  // Need to hold Ply and negative depths for qsearch too
using clock = std::chrono::steady_clock;

struct GoCmd {
    std::optional<Ply> depth;
    std::optional<Millis> wtime;
    std::optional<Millis> btime;
    std::optional<Millis> winc;
    std::optional<Millis> binc;
};

struct SearchInfo {
    std::array<Move, cfg::KILLERS_COUNT> killers;
    std::vector<Move> pv_line;

    SearchInfo() {
        std::fill(killers.begin(), killers.end(), move::Null);
    }
};

struct Searcher {
    Board &board;
    std::chrono::time_point<clock> clock_start;
    std::atomic<bool> stop_requested;
    Tt tt;
    ButterflyHistory butterfly_hist;
    Ply max_depth;  // max_depth always > 0
    Move bestmove;
    u64 node_cnt;
    u64 depth_one_node_cnt;
    Score root_score;
    Ply iter_depth;  // iter_depth always > 0
    Ply cur_ply;
    Millis max_millis;
    // Can exceed PLY_MAX due to uci moves
    std::vector<SearchInfo> stk;

    Searcher(Board &board);

    void new_game();

    void resize_tt(u64 mb);

    void allocate_time(GoCmd &cmd);

    void new_search(GoCmd &cmd);

    void reset_info();

    Millis elapsed();

    bool within_time_limit(Millis millis);

    void check_limits_reached();

    void make_move_end();

    void make_move(Move move);

    void unmake_move_end();

    void unmake_move();

    void update_pv_line(Move move);

    Score qsearch(Score alpha, Score beta);

    bool can_rfp(bool is_pv_node, Depth depth);

    Score rfp_margin(Depth depth);

    bool material_can_nmp();

    bool can_nmp(bool is_pv_node, Depth depth, Score eval, Score beta);

    Depth nmp_reduction(Depth depth);

    void make_null_move();

    void unmake_null_move();

    bool can_lmp(Depth depth, i32 moves_played);

    Depth extension(bool gives_check);

    bool can_lmr(Depth depth);

    Depth lmr(Depth depth, i32 moves_played, bool is_pv_node);

    void update_killers(Move move);

    i32 butterfly_history_bonus(Depth depth);

    HistoryScore clamp_history_score(i32 bonus);

    HistoryScore &get_butterfly_history(Move move);

    void update_butterfly_history(Move move, HistoryScore clamped_bonus);

    void update_butterfly_history(Move move, std::vector<Move> &quiets_played,
                                  Depth depth);

    void update_quiet_histories(Move move, std::vector<Move> &quiets_played,
                                Depth depth);

    Score pvs(Depth depth, Score alpha, Score beta);

    void aspiration_window(Depth depth);

    bool can_search_next_depth();

    std::string pv_str();

    void print_info();

    void update_bestmove();

    void iterative_deepening();

    void print_bestmove();

    void go(GoCmd cmd);

    void stop();
};

void init();

}  // namespace tuna::search

#endif
