#include "fsp/udp.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#error "Windows is not supported"
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fsp {

namespace {

int create_udp_socket() {
    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error(std::string("socket(): ") + std::strerror(errno));
    }

    int reuse = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        ::close(fd);
        throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR): ") + std::strerror(errno));
    }

#ifdef SO_REUSEPORT
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        ::close(fd);
        throw std::runtime_error(std::string("setsockopt(SO_REUSEPORT): ") + std::strerror(errno));
    }
#endif

    return fd;
}

sockaddr_in make_sockaddr_in(const std::string& address, std::uint16_t port) {
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (::inet_pton(AF_INET, address.c_str(), &sa.sin_addr) != 1) {
        throw std::runtime_error("inet_pton() failed for address: " + address);
    }
    return sa;
}

} // namespace

UdpReceiver::UdpReceiver() : fd_(create_udp_socket()) {}

UdpReceiver::~UdpReceiver() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

void UdpReceiver::bind(std::uint16_t port, const std::string& listen_interface) {
    const auto sa = make_sockaddr_in(listen_interface, port);
    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) < 0) {
        throw std::runtime_error(std::string("bind(): ") + std::strerror(errno));
    }
}

void UdpReceiver::join_multicast(const std::string& group, const std::string& interface_address) {
    ip_mreq mreq{};
    if (::inet_pton(AF_INET, group.c_str(), &mreq.imr_multiaddr) != 1) {
        throw std::runtime_error("inet_pton() failed for group: " + group);
    }

    if (::inet_pton(AF_INET, interface_address.c_str(), &mreq.imr_interface) != 1) {
        throw std::runtime_error("inet_pton() failed for interface: " + interface_address);
    }

    if (::setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw std::runtime_error(std::string("setsockopt(IP_ADD_MEMBERSHIP): ") + std::strerror(errno));
    }
}

std::vector<std::uint8_t> UdpReceiver::recv(std::size_t max_len) {
    std::vector<std::uint8_t> buf(max_len);

    const auto n = ::recvfrom(fd_, buf.data(), buf.size(), 0, nullptr, nullptr);
    if (n < 0) {
        throw std::runtime_error(std::string("recvfrom(): ") + std::strerror(errno));
    }

    buf.resize(static_cast<std::size_t>(n));
    return buf;
}

UdpSender::UdpSender() : fd_(create_udp_socket()) {}

UdpSender::~UdpSender() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

void UdpSender::send_to(const UdpEndpoint& ep, const std::vector<std::uint8_t>& bytes) {
    const auto sa = make_sockaddr_in(ep.address, ep.port);
    const auto n = ::sendto(fd_, bytes.data(), bytes.size(), 0, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
    if (n < 0) {
        throw std::runtime_error(std::string("sendto(): ") + std::strerror(errno));
    }
    if (static_cast<std::size_t>(n) != bytes.size()) {
        throw std::runtime_error("sendto(): short send");
    }
}

} // namespace fsp
