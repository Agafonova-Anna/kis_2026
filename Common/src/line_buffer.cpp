#include "common/line_buffer.hpp"

#include <unistd.h>

#include <cerrno>

namespace common {

bool LineBuffer::read_from(int fd) {
    char chunk[4096];
    const ssize_t n = ::read(fd, chunk, sizeof(chunk));
    if (n < 0 && (errno == EINTR || errno == EAGAIN)) {
        return true;
    }
    if (n <= 0) {
        return false;
    }
    data_.append(chunk, static_cast<std::size_t>(n));
    return data_.size() <= kMaxLine || data_.contains('\n');
}

std::optional<std::string> LineBuffer::pop_line() {
    const auto pos = data_.find('\n');
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    std::string line = data_.substr(0, pos);
    data_.erase(0, pos + 1);
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

} // namespace common
