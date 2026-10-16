#pragma once

#include <cstdint>
#include <string>

#include "common/socket.hpp"

namespace common {

Socket listen_tcp(std::uint16_t port);
Socket accept_connection(const Socket& listener);
Socket connect_tcp(const std::string& host, const std::string& port);

bool wait_readable(int fd, int timeout_ms);
void ignore_sigpipe();

} // namespace common
