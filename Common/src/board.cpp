#include "common/board.hpp"

namespace common {

std::optional<Board> Board::from_moves(std::string_view moves) {
    Board board;
    if (moves == "-") {
        return board;
    }
    for (const char c : moves) {
        if (c < '1' || c > '9' || !board.play(c - '0')) {
            return std::nullopt;
        }
    }
    return board;
}

bool Board::can_play(int cell) const {
    return cell >= 1 && cell <= kCells && at(cell) == Mark::None && !finished();
}

bool Board::play(int cell) {
    if (!can_play(cell)) {
        return false;
    }
    cells_[cell - 1] = turn();
    moves_.push_back(cell);
    return true;
}

Mark Board::winner() const {
    static constexpr int kLines[8][3] = {
        {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {1, 4, 7}, {2, 5, 8}, {3, 6, 9}, {1, 5, 9}, {3, 5, 7},
    };
    for (const auto& line : kLines) {
        const Mark mark = at(line[0]);
        if (mark != Mark::None && mark == at(line[1]) && mark == at(line[2])) {
            return mark;
        }
    }
    return Mark::None;
}

bool Board::finished() const {
    return moves_.size() == kCells || winner() != Mark::None;
}

std::string Board::moves_string() const {
    if (moves_.empty()) {
        return "-";
    }
    std::string result;
    for (const int cell : moves_) {
        result += static_cast<char>('0' + cell);
    }
    return result;
}

} // namespace common
