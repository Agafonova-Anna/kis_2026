#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/mark.hpp"

namespace common {

class Board {
public:
    static constexpr int kCells = 9;

    static std::optional<Board> from_moves(std::string_view moves);
    static Mark mark_of_move(std::size_t index) { return index % 2 == 0 ? Mark::X : Mark::O; }

    bool can_play(int cell) const;
    bool play(int cell);

    Mark at(int cell) const { return cells_[cell - 1]; }
    Mark turn() const { return mark_of_move(moves_.size()); }
    Mark winner() const;
    bool finished() const;
    const std::vector<int>& moves() const { return moves_; }

    std::string moves_string() const;

private:
    std::array<Mark, kCells> cells_{};
    std::vector<int> moves_;
};

} // namespace common
