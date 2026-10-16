#include <unistd.h>

#include <format>
#include <iostream>

#include "common/protocol.hpp"
#include "server/server.hpp"
#include "server/stats.hpp"

namespace server {

using common::Mark;

void Server::on_console_readable() {
    if (!console_.read_from(STDIN_FILENO)) {
        running_ = false;
        return;
    }
    while (auto line = console_.pop_line()) {
        on_console_line(*line);
    }
}

void Server::on_console_line(const std::string& line) {
    Args args = common::split(line);
    std::erase(args, std::string{});
    if (args.empty()) {
        return;
    }
    const std::string& command = args[0];
    if (command == "players") {
        cmd_players();
    } else if (command == "invite") {
        cmd_invite(args);
    } else if (command == "games") {
        cmd_games();
    } else if (command == "move") {
        cmd_move(args);
    } else if (command == "show") {
        cmd_show(args);
    } else if (command == "history") {
        cmd_history(args);
    } else if (command == "quit") {
        running_ = false;
    } else {
        std::cout << "Неизвестная команда" << std::endl;
    }
}

void Server::cmd_players() const {
    if (players_.empty()) {
        std::cout << "Пока ни один вариант не подключался." << std::endl;
        return;
    }
    for (const auto& [name, player] : players_) {
        std::cout << std::format("{}, {}: сыграно {}, побед {} ({}%), поражений {}, ничьих {}\n", name,
                                 player.online() ? "в сети" : "не в сети", player.played(),
                                 player.wins, percent(player.win_rate()), player.losses, player.draws);
    }
    std::cout << std::flush;
}

void Server::cmd_invite(const Args& args) {
    if (args.size() < 2 || args.size() > 3 || (args.size() == 3 && args[2] != "me" && args[2] != "loki")) {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }
    const auto it = players_.find(args[1]);
    if (it == players_.end()) {
        std::cout << std::format("Варианта {} нет.", args[1]) << std::endl;
        return;
    }
    Player& player = it->second;
    if (!player.online()) {
        std::cout << std::format("Вариант {} не в сети.", player.name) << std::endl;
        return;
    }

    Game& game = games_.emplace_back();
    game.id = static_cast<int>(games_.size());
    game.local_id = static_cast<int>(player.games.size()) + 1;
    game.loki = player.name;
    game.loki_mark = args.size() == 3 && args[2] == "loki" ? Mark::X : Mark::O;
    player.games.push_back(game.id);

    send_invite(game);
    std::cout << std::format("Приглашение в партию {} отправлено варианту {}. Первым ходит {}.", game.id,
                             player.name, game.mobius_turn() ? "Мобиус" : "вариант")
              << std::endl;
}

void Server::cmd_games() const {
    bool any = false;
    for (const Game& game : games_) {
        if (game.unfinished()) {
            std::cout << std::format("{}\n", game.describe());
            any = true;
        }
    }
    std::cout << (any ? "" : "Незаконченных партий нет.\n") << std::flush;
}

void Server::cmd_move(const Args& args) {
    if (args.size() != 3) {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }
    Game* game = find_game(args[1]);
    if (game == nullptr) {
        std::cout << std::format("Партии {} нет.", args[1]) << std::endl;
        return;
    }
    if (game->status != GameStatus::Active || !game->mobius_turn()) {
        std::cout << std::format("Сейчас нельзя ходить. {}", game->describe()) << std::endl;
        return;
    }
    const auto cell = common::parse_int(args[2]);
    if (!cell || !game->board.play(*cell)) {
        std::cout << "Недопустимый ход: клетка занята или вне 1-9." << std::endl;
        return;
    }

    std::cout << std::format("Ход в партии {} в клетку {}. Ждём подтверждения доставки.", game->id, *cell)
              << std::endl;
    send_snapshot(*game);
    if (game->board.finished()) {
        finish_game(*game);
    }
}

void Server::cmd_show(const Args& args) {
    if (args.size() != 2) {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }
    const Game* game = find_game(args[1]);
    if (game == nullptr) {
        std::cout << std::format("Партии {} нет.", args[1]) << std::endl;
        return;
    }
    std::cout << std::format("{}\nХоды: {}", game->describe(), game->moves_text()) << std::endl;
}

void Server::cmd_history(const Args& args) const {
    if (args.size() != 2) {
        std::cout << "Неверный формат команды" << std::endl;
        return;
    }
    const auto it = players_.find(args[1]);
    if (it == players_.end()) {
        std::cout << std::format("Варианта {} нет.", args[1]) << std::endl;
        return;
    }
    const Player& player = it->second;
    if (player.games.empty()) {
        std::cout << std::format("С вариантом {} ещё не играли.", player.name) << std::endl;
        return;
    }
    for (const int id : player.games) {
        const Game& game = games_[id - 1];
        std::cout << std::format("{}\n    ходы: {}\n", game.describe(), game.moves_text());
    }
    std::cout << std::format("Итого: побед варианта {}, поражений {}, ничьих {}", player.wins,
                             player.losses, player.draws)
              << std::endl;
}

Game* Server::find_game(const std::string& id) {
    const auto number = common::parse_int(id);
    if (!number || *number < 1 || *number > static_cast<int>(games_.size())) {
        return nullptr;
    }
    return &games_[*number - 1];
}

} // namespace server
