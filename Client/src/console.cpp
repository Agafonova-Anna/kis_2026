#include <unistd.h>

#include <format>
#include <iostream>

#include "client/client.hpp"
#include "common/protocol.hpp"

namespace client {

using common::Board;
using common::Mark;
namespace msg = common::msg;

void Client::on_console_readable() {
    if (!console_.read_from(STDIN_FILENO)) {
        running_ = false;
        return;
    }
    while (auto line = console_.pop_line()) {
        on_console_line(*line);
    }
}

void Client::on_console_line(const std::string& line) {
    Args args = common::split(line);
    std::erase(args, std::string{});
    if (args.empty()) {
        return;
    }
    const std::string& command = args[0];
    if (command == "accept" || command == "decline") {
        cmd_answer(args, command == "accept");
    } else if (command == "move") {
        cmd_move(args);
    } else if (command == "history") {
        cmd_history(args);
    } else if (command == "quit") {
        running_ = false;
    } else {
        std::cout << "Неизвестная команда" << std::endl;
    }
}

void Client::cmd_answer(const Args& args, bool accepted) {
    if (args.size() != 2) {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }
    const auto id = common::parse_int(args[1]);
    if (!id || !invites_.contains(*id)) {
        std::cout << "Такого приглашения нет." << std::endl;
        return;
    }
    if (!send(std::string(accepted ? msg::kAccept : msg::kDecline) + ' ' + std::to_string(*id))) {
        return;
    }
    invites_.erase(*id);
    std::cout << (accepted ? "Приглашение принято." : "Вы отказались. Мобиус это запомнит.")
              << std::endl;
}

void Client::cmd_move(const Args& args) {
    std::optional<int> id;
    std::string cell_arg;
    if (args.size() == 2) {
        id = current_game();
        cell_arg = args[1];
        if (!id) {
            std::cout << "Неверный формат команды" << std::endl;
            return;
        }
    } else if (args.size() == 3) {
        id = common::parse_int(args[1]);
        cell_arg = args[2];
    } else {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }

    const auto it = id ? games_.find(*id) : games_.end();
    if (it == games_.end()) {
        std::cout << "Такой партии нет." << std::endl;
        return;
    }
    const Game& game = it->second;
    if (game.board.finished()) {
        std::cout << std::format("Партия {} уже закончена.", *id) << std::endl;
        return;
    }
    if (game.board.turn() != game.mark) {
        std::cout << "Сейчас ход Мобиуса." << std::endl;
        return;
    }
    const auto cell = common::parse_int(cell_arg);
    if (!cell || !game.board.can_play(*cell)) {
        std::cout << "Недопустимый ход: клетка занята или вне 1-9." << std::endl;
        return;
    }
    send(std::string(msg::kMove) + ' ' + std::to_string(*id) + ' ' + std::to_string(*cell));
}

void Client::cmd_history(const Args& args) const {
    std::optional<int> id;
    if (args.size() == 2) {
        id = common::parse_int(args[1]);
    } else if (args.size() == 1) {
        id = current_game();
        if (!id && !games_.empty()) {
            id = games_.rbegin()->first;
        }
    } else {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }

    const auto it = id ? games_.find(*id) : games_.end();
    if (it == games_.end()) {
        std::cout << "Такой партии нет." << std::endl;
        return;
    }
    const Game& game = it->second;
    const auto& moves = game.board.moves();
    std::cout << std::format("Ходы партии {}, вы играете за {}:\n", *id, common::to_char(game.mark));
    if (moves.empty()) {
        std::cout << "ходов пока не было\n";
    }
    for (std::size_t i = 0; i < moves.size(); ++i) {
        std::cout << std::format("{}. {}, клетка {}\n", i + 1,
                                 Board::mark_of_move(i) == game.mark ? "Вы" : "Мобиус", moves[i]);
    }
    std::cout << std::flush;
}

std::optional<int> Client::current_game() const {
    std::optional<int> found;
    for (const auto& [id, game] : games_) {
        if (!game.board.finished()) {
            if (found) {
                return std::nullopt;
            }
            found = id;
        }
    }
    return found;
}

void Client::print_state(int id, const Game& game) const {
    const Mark winner = game.board.winner();
    if (!game.board.finished()) {
        if (game.board.turn() != game.mark) {
            std::cout << "Ждём ход Мобиуса.";
        } else if (current_game()) {
            std::cout << "Ваш ход: move <клетка>";
        } else {
            std::cout << std::format("Ваш ход в партии {}: move {} <клетка>", id, id);
        }
    } else if (winner == game.mark) {
        std::cout << std::format("Вы победили в партии {}. Пусть копии завидуют.", id);
    } else if (winner == Mark::None) {
        std::cout << std::format("Партия {}: ничья. Настоящий Локи довольствовался бы ничьёй?", id);
    } else {
        std::cout << std::format("Мобиус победил в партии {}. Ваши копии, наверное, справились бы лучше.", id);
    }
    std::cout << std::endl;
}

} // namespace client
