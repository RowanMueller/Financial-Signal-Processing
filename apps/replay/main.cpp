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
    std::filesystem::path in;
    std::string out_addr{"127.0.0.1"};
    std::uint16_t out_port{};
    bool realtime{false};
};

std::uint16_t parse_u16(const std::string& s) {
    const auto v = std::stoul(s);
    if (v > 65535) {
        throw std::runtime_error("value out of range: " + s);
    }
    return static_cast<std::uint16_t>(v);
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

        if (key == "--in") {
            a.in = need("--in");
        } else if (key == "--out-addr") {
            a.out_addr = need("--out-addr");
        } else if (key == "--out-port") {
            a.out_port = parse_u16(need("--out-port"));
        } else if (key == "--realtime") {
            a.realtime = true;
        } else if (key == "--help") {
            std::cout
                << "fsp_replay --in <file> --out-port <port> [--out-addr <addr>] [--realtime]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown arg: " + key);
        }
    }

    if (a.in.empty() || a.out_port == 0) {
        throw std::runtime_error("required: --in --out-port");
    }

    return a;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto args = parse_args(argc, argv);

        fsp::CaptureReader reader(args.in);
        fsp::UdpSender tx;

        std::cout << "replaying " << args.in.string() << " -> " << args.out_addr << ":" << args.out_port
                  << " realtime=" << (args.realtime ? "true" : "false") << "\n";

        std::uint64_t first_mono = 0;
        const std::uint64_t start_mono = fsp::monotonic_time_ns();

        std::uint64_t count = 0;
        while (auto rec = reader.next()) {
            if (count == 0) {
                first_mono = rec->mono_time_ns;
            }

            if (args.realtime) {
                const std::uint64_t target = start_mono + (rec->mono_time_ns - first_mono);
                for (;;) {
                    const auto now = fsp::monotonic_time_ns();
                    if (now >= target) {
                        break;
                    }
                    fsp::sleep_for_ns(target - now);
                }
            }

            tx.send_to(fsp::UdpEndpoint{args.out_addr, args.out_port}, rec->payload);
            ++count;

            if ((count % 100000) == 0) {
                std::cout << "sent " << count << " packets\n";
            }
        }

        std::cout << "done, sent " << count << " packets\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
