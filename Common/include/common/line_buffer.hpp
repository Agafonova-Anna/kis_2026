#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace common {

class LineBuffer {
public:
    static constexpr std::size_t kMaxLine = 4096;

    bool read_from(int fd);
    std::optional<std::string> pop_line();

private:
    std::string data_;
};

} // namespace common
