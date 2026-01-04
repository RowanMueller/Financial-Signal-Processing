#pragma once

#include <cstdint>

namespace fsp {

std::uint64_t monotonic_time_ns();
std::uint64_t wall_time_ns();

void sleep_for_ns(std::uint64_t ns);

} // namespace fsp
