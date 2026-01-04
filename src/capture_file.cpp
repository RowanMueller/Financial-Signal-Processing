#include "fsp/capture_file.hpp"

#include <array>
#include <stdexcept>

namespace fsp {

namespace {

constexpr std::array<char, 8> kMagic{'F','S','P','C','A','P','0','1'};

template <typename T>
void write_pod(std::ostream& os, const T& v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(T));
    if (!os) {
        throw std::runtime_error("failed to write capture file");
    }
}

template <typename T>
void read_pod(std::istream& is, T& v) {
    is.read(reinterpret_cast<char*>(&v), sizeof(T));
}

} // namespace

CaptureWriter::CaptureWriter(const std::filesystem::path& path)
    : out_(path, std::ios::binary | std::ios::out | std::ios::trunc) {
    if (!out_) {
        throw std::runtime_error("failed to open capture file for writing: " + path.string());
    }

    out_.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
    if (!out_) {
        throw std::runtime_error("failed to write capture header");
    }
}

void CaptureWriter::write(const CaptureRecord& record) {
    const std::uint32_t payload_len = static_cast<std::uint32_t>(record.payload.size());

    write_pod(out_, record.mono_time_ns);
    write_pod(out_, record.wall_time_ns);
    write_pod(out_, record.channel_id);
    write_pod(out_, payload_len);

    if (payload_len > 0) {
        out_.write(reinterpret_cast<const char*>(record.payload.data()), payload_len);
        if (!out_) {
            throw std::runtime_error("failed to write capture payload");
        }
    }
}

CaptureReader::CaptureReader(const std::filesystem::path& path)
    : in_(path, std::ios::binary | std::ios::in) {
    if (!in_) {
        throw std::runtime_error("failed to open capture file for reading: " + path.string());
    }

    std::array<char, 8> magic{};
    in_.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!in_) {
        throw std::runtime_error("failed to read capture header");
    }

    if (magic != kMagic) {
        throw std::runtime_error("invalid capture file magic");
    }
}

std::optional<CaptureRecord> CaptureReader::next() {
    CaptureRecord rec{};

    read_pod(in_, rec.mono_time_ns);
    if (!in_) {
        return std::nullopt;
    }

    read_pod(in_, rec.wall_time_ns);
    read_pod(in_, rec.channel_id);

    std::uint32_t payload_len = 0;
    read_pod(in_, payload_len);

    if (!in_) {
        throw std::runtime_error("truncated capture record header");
    }

    rec.payload.resize(payload_len);
    if (payload_len > 0) {
        in_.read(reinterpret_cast<char*>(rec.payload.data()), payload_len);
        if (!in_) {
            throw std::runtime_error("truncated capture record payload");
        }
    }

    return rec;
}

} // namespace fsp
