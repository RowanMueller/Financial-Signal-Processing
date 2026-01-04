#include "fsp/time.hpp"

#include <chrono>
#include <thread>

namespace fsp {

std::uint64_t monotonic_time_ns() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

std::uint64_t wall_time_ns() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

void sleep_for_ns(std::uint64_t ns) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
}

} // namespace fsp
