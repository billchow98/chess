#include "perf.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common.h"
#include "logging.h"

namespace tuna::perf {

std::unordered_map<std::string, u64> counters;
std::mutex mx;

void annotate() {
    auto sorted_counters = std::vector<std::pair<std::string, u64>>(
        counters.begin(), counters.end());
    std::sort(sorted_counters.begin(), sorted_counters.end());
    for (auto &[name, cnt] : sorted_counters) {
        logging::debug("{}: {}", name, cnt);
    }
}

}  // namespace tuna::perf
