#include "uci.h"

#include <sstream>
#include <string>
#include <thread>

#include "board.h"
#include "cfg.h"
#include "common.h"
#include "eval.h"
#include "io.h"
#include "movegen.h"
#include "search.h"

namespace tuna {
namespace uci_option {

enum { Spin };

std::string to_str(UciOptionType ot) {
    switch (ot) {
    case Spin:
        return "spin";
    default:
        unreachable();
    }
}

struct Option {
    std::string to_str() {
        std::string s;
        s += std::format("option name {} type {}", name,
                         uci_option::to_str(type));
        s += default_value ? std::format(" default {}", *default_value) : "";
        s += min_value ? std::format(" min {}", *min_value) : "";
        s += max_value ? std::format(" max {}", *max_value) : "";
        return s;
    }

    std::string name;
    UciOptionType type;
    std::optional<u64> default_value;
    std::optional<u64> min_value;
    std::optional<u64> max_value;
};

const auto OPTIONS = std::array{
    Option{"Hash", Spin, 16, 1, std::numeric_limits<u64>::max()},
};

}  // namespace uci_option

namespace uci {

using board::Board;
using search::Searcher;

Board board;
Searcher searcher(board);
std::thread search_thread;
std::atomic<bool> quit_requested = false;

void init() {
    // Hashes aren't initialized before this. As such, we must reinitialize
    // the board (and not rely on static initialization with hash = 0)
    board = Board();
    io::println("{} by {}", cfg::UCI_NAME, cfg::UCI_AUTHOR);
}

std::istringstream get_input() {
    std::string input;
    std::getline(std::cin, input);
    return std::istringstream{input};
}

void print_uci_options() {
    for (auto option : uci_option::OPTIONS) {
        io::println("{}", option.to_str());
    }
}

void handle_uci() {
    io::println("id name {}", cfg::UCI_NAME);
    io::println("id author {}", cfg::UCI_AUTHOR);
    print_uci_options();
    io::println("uciok");
}

void handle_setoption(std::istringstream &iss) {
    std::string token;
    if (iss >> token && token != "name") {
        return;
    }
    if (!(iss >> token)) {
        return;
    }
    if (token == "Hash") {
        if (iss >> token && token != "value") {
            return;
        }
        u64 mb;
        if (!(iss >> mb)) {
            return;
        }
        searcher.resize_tt(mb);
    }
}

void handle_ucinewgame() {
    searcher.new_game();
}

void handle_position(std::istringstream &iss) {
    using board::PositionCmd;
    std::string token;
    PositionCmd cmd;
    if (!(iss >> token)) {
        return;
    }
    if (token == "startpos") {
        cmd.type = PositionCmd::Startpos;
    } else if (token == "fen") {
        cmd.type = PositionCmd::Fen;
        for (auto i = 0; i < 6; i++) {
            if (!(iss >> token)) {
                return;
            }
            if (i > 0) {
                cmd.fen += ' ';
            }
            cmd.fen += token;
        }
    } else {
        return;
    }
    if (iss >> token && token != "moves") {
        return;
    }
    while (iss >> token) {
        auto move = move::from_str(token);
        if (move == move::Null) {
            return;
        }
        cmd.moves.push_back(move);
    }
    board.setup(cmd);
}

void handle_go(std::istringstream &iss) {
    using search::GoCmd;
    std::string token;
    GoCmd cmd;
    while (iss >> token) {
        if (token == "depth") {
            i32 depth;
            if (!(iss >> depth)) {
                return;
            }
            cmd.depth = depth;
        } else if (token == "wtime") {
            if (!(iss >> *cmd.wtime)) {
                return;
            }
            // Needed so that has_value() is true
            cmd.wtime = *cmd.wtime;
        } else if (token == "btime") {
            if (!(iss >> *cmd.btime)) {
                return;
            }
            cmd.btime = *cmd.btime;
        } else if (token == "winc") {
            if (!(iss >> *cmd.winc)) {
                return;
            }
            cmd.winc = *cmd.winc;
        } else if (token == "binc") {
            if (!(iss >> *cmd.binc)) {
                return;
            }
            cmd.binc = *cmd.binc;
        } else {
            return;
        }
    }
    if (search_thread.joinable()) {
        search_thread.join();
    }
    search_thread = std::thread([cmd] { searcher.go(cmd); });
}

void handle_stop() {
    searcher.stop();
}

void handle_perft(std::istringstream &iss) {
    i32 depth;
    if (!(iss >> depth)) {
        return;
    }
    movegen::perft(board, depth);
}

void handle_board() {
    io::println("{}", board.debug_str());
}

void handle_eval() {
    io::println("{}", eval::evaluate(board));
}

void handle_command() {
    std::string token;
    auto iss = get_input();
    iss >> token;
    if (token == "uci") {
        handle_uci();
    } else if (token == "isready") {
        io::println("readyok");
    } else if (token == "setoption") {
        handle_setoption(iss);
    } else if (token == "ucinewgame") {
        handle_ucinewgame();
    } else if (token == "position") {
        handle_position(iss);
    } else if (token == "go") {
        handle_go(iss);
    } else if (token == "stop") {
        handle_stop();
    } else if (token == "quit") {
        quit_requested = true;
    } else if (cfg::DEVEL) {
        if (token == "perft") {
            handle_perft(iss);
        } else if (token == "board") {
            handle_board();
        } else if (token == "eval") {
            handle_eval();
        }
    }
}

i32 run_loop() {
    init();
    while (!quit_requested) {
        handle_command();
    }
    if (search_thread.joinable()) {
        search_thread.join();
    }
    return 0;
}

}  // namespace uci
}  // namespace tuna
