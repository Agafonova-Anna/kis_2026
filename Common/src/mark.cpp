#include "common/mark.hpp"

namespace common {

char to_char(Mark mark) {
    switch (mark) {
    case Mark::X:
        return 'X';
    case Mark::O:
        return 'O';
    default:
        return ' ';
    }
}

Mark opposite(Mark mark) {
    switch (mark) {
    case Mark::X:
        return Mark::O;
    case Mark::O:
        return Mark::X;
    default:
        return Mark::None;
    }
}

std::optional<Mark> parse_mark(std::string_view text) {
    if (text == "X") {
        return Mark::X;
    }
    if (text == "O") {
        return Mark::O;
    }
    return std::nullopt;
}

} // namespace common
