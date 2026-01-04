#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fsp {

struct UdpEndpoint {
    std::string address;
    std::uint16_t port{};
};

class UdpReceiver {
public:
    UdpReceiver();
    ~UdpReceiver();

    UdpReceiver(const UdpReceiver&) = delete;
    UdpReceiver& operator=(const UdpReceiver&) = delete;

    void bind(std::uint16_t port, const std::string& listen_interface = "0.0.0.0");
    void join_multicast(const std::string& group, const std::string& interface_address = "0.0.0.0");

    std::vector<std::uint8_t> recv(std::size_t max_len);

private:
    int fd_{-1};
};

class UdpSender {
public:
    UdpSender();
    ~UdpSender();

    UdpSender(const UdpSender&) = delete;
    UdpSender& operator=(const UdpSender&) = delete;

    void send_to(const UdpEndpoint& ep, const std::vector<std::uint8_t>& bytes);

private:
    int fd_{-1};
};

} // namespace fsp
