#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "common/line_buffer.hpp"
#include "common/socket.hpp"

namespace common {

class Connection {
public:
    explicit Connection(Socket socket) : socket_(std::move(socket)) {}

    int fd() const { return socket_.fd(); }
    bool send_line(std::string_view line);
    bool receive() { return input_.read_from(socket_.fd()); }
    std::optional<std::string> next_line() { return input_.pop_line(); }
    void shutdown();

private:
    Socket socket_;
    LineBuffer input_;
};

} // namespace common
