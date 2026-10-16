#include "server/game.hpp"

#include <format>

namespace server {

using common::Board;
using common::Mark;

bool Game::delivered() const {
    std::size_t required = board.moves().size();
    if (required > 0 && Board::mark_of_move(required - 1) != mobius_mark()) {
        --required;
    }
    return acked >= required;
}

std::string Game::status_text() const {
    switch (status) {
    case GameStatus::Invited:
        return "ждём ответа на приглашение";
    case GameStatus::Declined:
        return "приглашение отклонено";
    case GameStatus::Active:
        return mobius_turn() ? "ход Мобиуса" : "ход варианта";
    case GameStatus::Abandoned:
        return "прервана, вариант отключился";
    case GameStatus::Finished:
        break;
    }
    const Mark winner = board.winner();
    if (winner == Mark::None) {
        return "ничья";
    }
    return winner == mobius_mark() ? "победил Мобиус" : "победил вариант";
}

std::string Game::describe() const {
    return std::format("Партия {} с {}, Мобиус играет за {}: {}{}", id, loki,
                       common::to_char(mobius_mark()), status_text(),
                       delivered() ? "" : ", последний ход Мобиуса не подтверждён");
}

std::string Game::moves_text() const {
    const auto& moves = board.moves();
    if (moves.empty()) {
        return "ходов не было";
    }
    std::string text;
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (i > 0) {
            text += ", ";
        }
        text += Board::mark_of_move(i) == mobius_mark() ? "Мобиус " : loki + ' ';
        text += std::to_string(moves[i]);
    }
    return text;
}

} // namespace server
