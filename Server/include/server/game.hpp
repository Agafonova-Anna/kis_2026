#pragma once

#include <cstddef>
#include <string>

#include "common/board.hpp"
#include "common/mark.hpp"

namespace server {

enum class GameStatus { Invited, Declined, Active, Finished, Abandoned };

struct Game {
    int id = 0;
    int local_id = 0;
    std::string loki;
    common::Mark loki_mark = common::Mark::O;
    GameStatus status = GameStatus::Invited;
    common::Board board;
    std::size_t acked = 0;

    common::Mark mobius_mark() const { return common::opposite(loki_mark); }
    bool mobius_turn() const { return board.turn() == mobius_mark(); }
    bool unfinished() const { return status == GameStatus::Invited || status == GameStatus::Active; }
    bool delivered() const;

    std::string status_text() const;
    std::string describe() const;
    std::string moves_text() const;
};

} // namespace server
