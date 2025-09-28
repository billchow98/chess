#ifndef TUNA_PERF_H
#define TUNA_PERF_H

#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "common.h"

namespace tuna::perf {

extern std::unordered_map<std::string, u64> counters;
extern std::mutex mx;

template<class... Args>
void record(std::format_string<Args...> format, Args &&...args) {
    auto name = std::format(format, std::forward<Args>(args)...);
    std::lock_guard<std::mutex> lock(mx);
    counters[name]++;
}

void annotate();

}  // namespace tuna::perf

#endif
