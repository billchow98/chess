#ifndef TUNA_CFG_H
#define TUNA_CFG_H

#include <format>
#include <string>

namespace tuna::cfg {

const auto UCI_NAME = std::format("tuna {}", TUNA_VERSION);
const auto UCI_AUTHOR = "Bill Chow";
const auto DEVEL = UCI_NAME.find('-') != std::string::npos;
const auto MOVE_VEC_RESERVE_CAP = 32;
const auto KILLERS_COUNT = 2;
const auto SEARCH_POLL_NODE_FREQ = 1'024;
const auto ASP_WINDOW_SIZE = 10;
const auto UCI_LATENCY_MS = 5;

}  // namespace tuna::cfg

#endif
