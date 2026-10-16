#include "common/socket.hpp"

#include <unistd.h>

#include <utility>

namespace common {

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    close();
    fd_ = std::exchange(other.fd_, -1);
    return *this;
}

void Socket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

} // namespace common
