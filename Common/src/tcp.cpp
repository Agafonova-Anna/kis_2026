#include "common/tcp.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace common {

namespace {

constexpr int kSendTimeoutSec = 2;

std::runtime_error system_error(const std::string& what) {
    return std::runtime_error(what + ": " + std::strerror(errno));
}

void configure_stream(int fd) {
    int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    timeval timeout{kSendTimeoutSec, 0};
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

} // namespace

Socket listen_tcp(std::uint16_t port) {
    Socket socket(::socket(AF_INET, SOCK_STREAM, 0));
    if (!socket.valid()) {
        throw system_error("socket");
    }
    int yes = 1;
    ::setsockopt(socket.fd(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (::bind(socket.fd(), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw system_error("bind");
    }
    if (::listen(socket.fd(), SOMAXCONN) < 0) {
        throw system_error("listen");
    }
    return socket;
}

Socket accept_connection(const Socket& listener) {
    Socket socket(::accept(listener.fd(), nullptr, nullptr));
    if (!socket.valid()) {
        throw system_error("accept");
    }
    configure_stream(socket.fd());
    return socket;
}

Socket connect_tcp(const std::string& host, const std::string& port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* found = nullptr;
    if (const int rc = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &found); rc != 0) {
        throw std::runtime_error(std::string("getaddrinfo: ") + ::gai_strerror(rc));
    }
    const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> guard(found, &::freeaddrinfo);

    for (const addrinfo* ai = found; ai != nullptr; ai = ai->ai_next) {
        Socket socket(::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
        if (socket.valid() && ::connect(socket.fd(), ai->ai_addr, ai->ai_addrlen) == 0) {
            configure_stream(socket.fd());
            return socket;
        }
    }
    throw system_error("connect");
}

bool wait_readable(int fd, int timeout_ms) {
    pollfd pfd{fd, POLLIN, 0};
    int rc = 0;
    do {
        rc = ::poll(&pfd, 1, timeout_ms);
    } while (rc < 0 && errno == EINTR);
    return rc > 0;
}

void ignore_sigpipe() {
    std::signal(SIGPIPE, SIG_IGN);
}

} // namespace common
