#pragma once

namespace common {

class Socket {
public:
    Socket() = default;
    explicit Socket(int fd) : fd_(fd) {}

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    ~Socket();

    int fd() const { return fd_; }
    bool valid() const { return fd_ >= 0; }
    void close();

private:
    int fd_ = -1;
};

} // namespace common
