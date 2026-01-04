#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace fsp {

struct CaptureRecord {
    std::uint64_t mono_time_ns{};
    std::uint64_t wall_time_ns{};
    std::uint32_t channel_id{};
    std::vector<std::uint8_t> payload;
};

class CaptureWriter {
public:
    explicit CaptureWriter(const std::filesystem::path& path);

    void write(const CaptureRecord& record);

private:
    std::ofstream out_;
};

class CaptureReader {
public:
    explicit CaptureReader(const std::filesystem::path& path);

    std::optional<CaptureRecord> next();

private:
    std::ifstream in_;
};

} // namespace fsp
