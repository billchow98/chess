#ifndef TUNA_LOGGING_H
#define TUNA_LOGGING_H

#include <format>
#include <iostream>
#include <utility>

#include "io.h"

namespace tuna::logging {

std::string timestamp();

template<class... Args>
void debug(std::format_string<Args...> format, Args &&...args) {
    auto msg = std::format(format, std::forward<Args>(args)...);
    io::println(std::cerr, "[{}][Debug] {}", timestamp(), msg);
}

}  // namespace tuna::logging

#endif
