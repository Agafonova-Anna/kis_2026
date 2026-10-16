#pragma once

#include <optional>
#include <string_view>

namespace common {

enum class Mark : char { None, X, O };

char to_char(Mark mark);
Mark opposite(Mark mark);
std::optional<Mark> parse_mark(std::string_view text);

} // namespace common
