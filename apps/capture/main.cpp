#include "fsp/capture_file.hpp"
#include "fsp/time.hpp"
#include "fsp/udp.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Args {
    std::string group;
    std::uint16_t port{};
    std::string iface{"0.0.0.0"};
    std::uint32_t channel_id{0};
    std::filesystem::path out;
    std::size_t max_len{2048};
    std::uint64_t count{0};
    bool join_multicast{true};
};

std::uint16_t parse_u16(const std::string& s) {
    const auto v = std::stoul(s);
    if (v > 65535) {
        throw std::runtime_error("value out of range: " + s);
    }
    return static_cast<std::uint16_t>(v);
}

std::uint32_t parse_u32(const std::string& s) {
    const auto v = std::stoul(s);
    if (v > 0xFFFFFFFFu) {
        throw std::runtime_error("value out of range: " + s);
    }
    return static_cast<std::uint32_t>(v);
}

std::size_t parse_size(const std::string& s) {
    return static_cast<std::size_t>(std::stoull(s));
}

std::uint64_t parse_u64(const std::string& s) {
    return static_cast<std::uint64_t>(std::stoull(s));
}

Args parse_args(int argc, char** argv) {
    Args a;

    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        auto need = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++i];
        };

        if (key == "--group") {
            a.group = need("--group");
        } else if (key == "--port") {
            a.port = parse_u16(need("--port"));
        } else if (key == "--iface") {
            a.iface = need("--iface");
        } else if (key == "--channel") {
            a.channel_id = parse_u32(need("--channel"));
        } else if (key == "--out") {
            a.out = need("--out");
        } else if (key == "--maxlen") {
            a.max_len = parse_size(need("--maxlen"));
        } else if (key == "--count") {
            a.count = parse_u64(need("--count"));
        } else if (key == "--no-join") {
            a.join_multicast = false;
        } else if (key == "--help") {
            std::cout
                << "fsp_capture --group <addr> --port <port> --out <file> [--iface <ifaddr>] [--channel <id>] [--maxlen <bytes>] [--count <n>] [--no-join]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown arg: " + key);
        }
    }

    if (a.group.empty() || a.port == 0 || a.out.empty()) {
        throw std::runtime_error("required: --group --port --out");
    }

    return a;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto args = parse_args(argc, argv);

        fsp::UdpReceiver rx;
        rx.bind(args.port);
        if (args.join_multicast) {
            rx.join_multicast(args.group, args.iface);
        }

        fsp::CaptureWriter writer(args.out);

        std::cout << "capturing " << args.group << ":" << args.port << " iface=" << args.iface
                  << " -> " << args.out.string() << "\n";

        std::uint64_t captured = 0;
        for (;;) {
            auto bytes = rx.recv(args.max_len);

            fsp::CaptureRecord rec;
            rec.mono_time_ns = fsp::monotonic_time_ns();
            rec.wall_time_ns = fsp::wall_time_ns();
            rec.channel_id = args.channel_id;
            rec.payload = std::move(bytes);

            writer.write(rec);

            ++captured;
            if (args.count > 0 && captured >= args.count) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
