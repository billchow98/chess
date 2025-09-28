#include "logging.h"

#include <chrono>
#include <format>
#include <string>

namespace tuna::logging {

std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    return std::format("{:%Y-%m-%d %H:%M:%S}", now);
}

}  // namespace tuna::logging
