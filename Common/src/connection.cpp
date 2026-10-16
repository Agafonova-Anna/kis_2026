#include "common/connection.hpp"

#include <sys/socket.h>

#include <cerrno>

namespace common {

bool Connection::send_line(std::string_view line) {
    std::string data(line);
    data += '\n';
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t n = ::send(socket_.fd(), data.data() + sent, data.size() - sent, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

void Connection::shutdown() {
    ::shutdown(socket_.fd(), SHUT_RDWR);
}

} // namespace common
