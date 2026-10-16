#include "common/protocol.hpp"

#include <charconv>
#include <ranges>

namespace common {

std::vector<std::string> split(std::string_view line) {
    return line
        | std::views::split(std::string_view(" "))
        | std::ranges::to<std::vector<std::string>>()
        ;
}

std::optional<int> parse_int(std::string_view text) {
    int value = 0;
    const char* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

bool is_valid_name(std::string_view name) {
    if (name.empty() || name.size() > kMaxNameLength) {
        return false;
    }
    return true;
}

} // namespace common
