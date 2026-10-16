#include "server/server.hpp"

#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>

#include "common/protocol.hpp"
#include "common/tcp.hpp"
#include "server/stats.hpp"

namespace server {

using common::Board;
using common::Mark;
namespace msg = common::msg;

namespace {

std::string error_line(const std::string& text) {
    return std::string(msg::kError) + ' ' + text;
}

} // namespace

Server::Server(std::uint16_t port) : listener_(common::listen_tcp(port)) {
    std::cout << std::format("Сервер Мобиуса слушает порт {}", port) << std::endl;
}

void Server::run() {
    while (running_) {
        std::vector<pollfd> fds = collect_fds();
        poll_and_dispatch(fds);
    }
}

std::vector<pollfd> Server::collect_fds() const {
    std::vector<pollfd> fds;
    for (const auto& [fd, session] : sessions_) {
        fds.push_back({fd, POLLIN, 0});
    }
    fds.push_back({listener_.fd(), POLLIN, 0});
    fds.push_back({STDIN_FILENO, POLLIN, 0});
    return fds;
}

void Server::poll_and_dispatch(std::vector<pollfd>& fds) {
    if (::poll(fds.data(), fds.size(), -1) < 0) {
        if (errno == EINTR) {
            return;
        }
        throw std::runtime_error(std::string("poll: ") + std::strerror(errno));
    }
    for (const pollfd& p : fds) {
        if (p.revents == 0) {
            continue;
        }
        if (p.fd == STDIN_FILENO) {
            on_console_readable();
        } else if (p.fd == listener_.fd()) {
            accept_session();
        } else {
            on_session_readable(p.fd);
        }
    }
}

void Server::accept_session() {
    try {
        common::Socket socket = common::accept_connection(listener_);
        const int fd = socket.fd();
        sessions_.emplace(fd, Session{common::Connection(std::move(socket)), {}});
    } catch (const std::exception& e) {
        std::cout << std::format("Не удалось принять соединение: {}", e.what()) << std::endl;
    }
}

void Server::on_session_readable(int fd) {
    const auto it = sessions_.find(fd);
    if (it == sessions_.end()) {
        return;
    }
    Session& session = it->second;
    bool alive = session.connection.receive();
    while (auto line = session.connection.next_line()) {
        if (!handle_message(session, *line)) {
            alive = false;
            break;
        }
    }
    if (!alive) {
        drop_session(fd);
    }
}

void Server::drop_session(int fd) {
    const auto it = sessions_.find(fd);
    if (it == sessions_.end()) {
        return;
    }
    if (!it->second.player.empty()) {
        Player& player = players_.at(it->second.player);
        player.session = -1;
        for (const int id : player.games) {
            Game& game = games_[id - 1];
            if (game.unfinished()) {
                game.status = GameStatus::Abandoned;
            }
        }
        std::cout << std::format("Вариант {} отключился", player.name) << std::endl;
    }
    sessions_.erase(it);
}

bool Server::handle_message(Session& session, const std::string& line) {
    Args args = common::split(line);
    std::erase(args, std::string{});
    if (args.empty()) {
        return true;
    }
    const std::string& command = args[0];
    if (command == msg::kHello) {
        return handle_hello(session, args);
    }
    if (session.player.empty()) {
        session.connection.send_line(error_line("сначала представьтесь: HELLO <имя>"));
        return false;
    }

    Player& player = players_.at(session.player);
    if (command == msg::kAccept || command == msg::kDecline) {
        handle_answer(player, args, command == msg::kAccept);
    } else if (command == msg::kMove) {
        handle_move(player, args);
    } else if (command == msg::kAck) {
        handle_ack(player, args);
    } else {
        send_to(player, error_line("неизвестная команда"));
    }
    return true;
}

bool Server::handle_hello(Session& session, const Args& args) {
    if (!session.player.empty()) {
        send_to(players_.at(session.player), error_line("вы уже представились"));
        return true;
    }
    if (args.size() != 2 || !common::is_valid_name(args[1])) {
        session.connection.send_line(error_line("недопустимое имя"));
        return false;
    }

    const auto [it, created] = players_.try_emplace(args[1]);
    if (!created) {
        session.connection.send_line(error_line("это имя уже занято, выберите другое"));
        return false;
    }
    Player& player = it->second;
    player.name = args[1];
    player.session = session.connection.fd();
    session.player = player.name;
    session.connection.send_line(std::string(msg::kWelcome));
    std::cout << std::format("Подключился новый вариант: {}", player.name) << std::endl;
    return true;
}

void Server::handle_answer(Player& player, const Args& args, bool accepted) {
    Game* game = args.size() == 2 ? variant_game(player, args[1]) : nullptr;
    if (game == nullptr || game->status != GameStatus::Invited) {
        send_to(player, error_line("такого приглашения нет"));
        return;
    }
    if (!accepted) {
        game->status = GameStatus::Declined;
        std::cout << std::format("Вариант {} отклонил приглашение в партию {}", player.name, game->id)
                  << std::endl;
        return;
    }
    game->status = GameStatus::Active;
    std::cout << std::format("Вариант {} принял приглашение в партию {}. {}", player.name, game->id,
                             game->mobius_turn()
                                 ? std::format("Ваш ход: move {} <клетка>", game->id)
                                 : std::string("Первым ходит вариант."))
              << std::endl;
    send_snapshot(*game);
}

void Server::handle_move(Player& player, const Args& args) {
    Game* game = args.size() == 3 ? variant_game(player, args[1]) : nullptr;
    if (game == nullptr || game->status != GameStatus::Active) {
        send_to(player, error_line("такой партии нет или она не идёт"));
        return;
    }
    if (game->mobius_turn()) {
        send_to(player, error_line("сейчас ход Мобиуса"));
        return;
    }
    const auto cell = common::parse_int(args[2]);
    if (!cell || !game->board.play(*cell)) {
        send_to(player, error_line("недопустимый ход"));
        return;
    }

    std::cout << std::format("Вариант {} сходил в партии {} в клетку {}", player.name, game->id, *cell)
              << std::endl;
    send_snapshot(*game);
    if (game->board.finished()) {
        finish_game(*game);
    } else {
        std::cout << std::format("Ваш ход: move {} <клетка>", game->id) << std::endl;
    }
}

void Server::handle_ack(Player& player, const Args& args) {
    Game* game = args.size() == 3 ? variant_game(player, args[1]) : nullptr;
    const auto count = args.size() == 3 ? common::parse_int(args[2]) : std::nullopt;
    if (game == nullptr || !count || *count < 0) {
        send_to(player, error_line("некорректное подтверждение"));
        return;
    }
    const auto& moves = game->board.moves();
    const std::size_t confirmed = std::min(static_cast<std::size_t>(*count), moves.size());
    for (std::size_t i = game->acked; i < confirmed; ++i) {
        if (Board::mark_of_move(i) == game->mobius_mark()) {
            std::cout << std::format("Ход Мобиуса в партии {} (клетка {}) доставлен варианту {}",
                                     game->id, moves[i], player.name)
                      << std::endl;
        }
    }
    game->acked = std::max(game->acked, confirmed);
}

void Server::send_to(const Player& player, const std::string& line) {
    if (!player.online()) {
        return;
    }
    common::Connection& connection = sessions_.at(player.session).connection;
    if (!connection.send_line(line)) {
        connection.shutdown();
    }
}

void Server::send_invite(const Game& game) {
    const Player& player = players_.at(game.loki);
    send_to(player, std::string(msg::kInvite) + ' ' + std::to_string(game.local_id) + ' ' +
                        common::to_char(game.loki_mark) + ' ' + envy_message(player, players_));
}

void Server::send_snapshot(const Game& game) {
    send_to(players_.at(game.loki), std::string(msg::kGame) + ' ' + std::to_string(game.local_id) +
                                        ' ' + common::to_char(game.loki_mark) + ' ' +
                                        game.board.moves_string());
}

void Server::finish_game(Game& game) {
    game.status = GameStatus::Finished;
    Player& player = players_.at(game.loki);
    const Mark winner = game.board.winner();
    if (winner == Mark::None) {
        ++player.draws;
    } else if (winner == game.loki_mark) {
        ++player.wins;
    } else {
        ++player.losses;
    }
    std::cout << std::format("Партия {} с {} завершена: {}", game.id, game.loki, game.status_text())
              << std::endl;
}

Game* Server::variant_game(const Player& player, const std::string& local_id) {
    const auto id = common::parse_int(local_id);
    if (!id || *id < 1 || *id > static_cast<int>(player.games.size())) {
        return nullptr;
    }
    return &games_[player.games[*id - 1] - 1];
}

} // namespace server
