#ifndef TUNA_IO_H
#define TUNA_IO_H

#include <format>
#include <iostream>
#include <mutex>
#include <utility>

namespace tuna::io {

extern std::mutex mx;

template<class... Args>
void println(std::ostream &os, std::format_string<Args...> format,
             Args &&...args) {
    std::lock_guard<std::mutex> lock(mx);
    os << std::format(format, std::forward<Args>(args)...) << std::endl;
}

template<class... Args>
void println(std::format_string<Args...> format, Args &&...args) {
    println(std::cout, format, std::forward<Args>(args)...);
}

}  // namespace tuna::io

#endif
