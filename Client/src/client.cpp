#include "client/client.hpp"

#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>

#include "common/protocol.hpp"
#include "common/tcp.hpp"

namespace client {

using common::Board;
using common::Mark;
namespace msg = common::msg;

namespace {

constexpr int kReplyTimeoutMs = 5000;

std::string read_reply(common::Connection& connection) {
    while (true) {
        if (auto line = connection.next_line()) {
            return *line;
        }
        if (!common::wait_readable(connection.fd(), kReplyTimeoutMs)) {
            throw std::runtime_error("сервер не отвечает");
        }
        if (!connection.receive()) {
            throw std::runtime_error("сервер закрыл соединение");
        }
    }
}

std::string join_from(const std::vector<std::string>& parts, std::size_t from) {
    std::string text;
    for (std::size_t i = from; i < parts.size(); ++i) {
        if (!text.empty()) {
            text += ' ';
        }
        text += parts[i];
    }
    return text;
}

} // namespace

Client::Client(std::string host, std::string port, std::string name)
    : host_(std::move(host)), port_(std::move(port)), name_(std::move(name)) {}

void Client::run() {
    if (!connect()) {
        return;
    }

    while (running_) {
        std::vector<pollfd> fds = collect_fds();
        poll_and_dispatch(fds);
    }
}

std::vector<pollfd> Client::collect_fds() const {
    return {{STDIN_FILENO, POLLIN, 0}, {connection_->fd(), POLLIN, 0}};
}

void Client::poll_and_dispatch(std::vector<pollfd>& fds) {
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
        } else if (connection_) {
            on_server_readable();
        }
    }
}

bool Client::connect() {
    try {
        common::Connection connection(common::connect_tcp(host_, port_));
        if (!connection.send_line(std::string(msg::kHello) + ' ' + name_)) {
            throw std::runtime_error("не удалось отправить приветствие");
        }

        Args reply = common::split(read_reply(connection));
        std::erase(reply, std::string{});
        if (!reply.empty() && reply[0] == msg::kWelcome) {
            connection_ = std::move(connection);
            std::cout << std::format("Подключено к серверу Мобиуса как {}", name_) << std::endl;
            while (connection_) {
                auto line = connection_->next_line();
                if (!line) {
                    break;
                }
                on_server_line(*line);
            }
            return true;
        }

        std::cout << std::format("Сервер отказал в подключении: {}",
                                 !reply.empty() && reply[0] == msg::kError
                                     ? join_from(reply, 1)
                                     : std::string("неожиданный ответ"))
                  << std::endl;
    } catch (const std::exception& e) {
        std::cout << std::format("Не удалось подключиться к {}:{}: {}", host_, port_, e.what())
                  << std::endl;
    }
    return false;
}

void Client::on_server_readable() {
    const bool alive = connection_->receive();
    while (connection_) {
        auto line = connection_->next_line();
        if (!line) {
            break;
        }
        on_server_line(*line);
    }
    if (!alive) {
        connection_lost();
    }
}

void Client::connection_lost() {
    if (!connection_) {
        return;
    }
    connection_.reset();
    running_ = false;
    std::cout << "Связь с сервером потеряна" << std::endl;
}

bool Client::send(const std::string& line) {
    if (!connection_) {
        return false;
    }
    if (!connection_->send_line(line)) {
        connection_lost();
        return false;
    }
    return true;
}

void Client::on_server_line(const std::string& line) {
    Args parts = common::split(line);
    std::erase(parts, std::string{});
    if (parts.empty()) {
        return;
    }
    if (parts[0] == msg::kInvite) {
        on_invite(parts);
    } else if (parts[0] == msg::kGame) {
        on_game(parts);
    } else if (parts[0] == msg::kError) {
        std::cout << std::format("Сервер: {}",
                                 parts.size() > 1 ? join_from(parts, 1) : std::string("ошибка"))
                  << std::endl;
    }
}

void Client::on_invite(const Args& parts) {
    if (parts.size() < 4) {
        return;
    }
    const auto id = common::parse_int(parts[1]);
    const auto mark = common::parse_mark(parts[2]);
    if (!id || !mark) {
        return;
    }
    const std::string message = join_from(parts, 3);
    invites_.insert(*id);
    std::cout << std::format("Мобиус приглашает вас на партию {0}. Вы играете за {1}, первым ходит {2}.\n"
                             "{3}\nЧтобы принять, введите accept {0}, чтобы отказаться, decline {0}",
                             *id, common::to_char(*mark), *mark == Mark::X ? "вариант" : "Мобиус",
                             message)
              << std::endl;
}

void Client::on_game(const Args& parts) {
    if (parts.size() != 4) {
        return;
    }
    const auto id = common::parse_int(parts[1]);
    const auto mark = common::parse_mark(parts[2]);
    const auto board = Board::from_moves(parts[3]);
    if (!id || !mark || !board) {
        return;
    }
    invites_.erase(*id);

    const auto [it, created] = games_.try_emplace(*id, Game{*mark, {}});
    Game& game = it->second;
    const auto& moves = board->moves();
    const std::size_t known = game.board.moves().size();
    if (created) {
        std::cout << std::format("Партия {} началась. Вы играете за {}", *id, common::to_char(*mark))
                  << std::endl;
    }
    for (std::size_t i = known; i < moves.size(); ++i) {
        std::cout << std::format("Партия {}: {} в клетку {}", *id,
                                 Board::mark_of_move(i) == game.mark ? "вы сходили" : "Мобиус сходил",
                                 moves[i])
                  << std::endl;
    }
    if (created || moves.size() != known) {
        game.board = *board;
        print_state(*id, game);
    }
    send(std::string(msg::kAck) + ' ' + std::to_string(*id) + ' ' + std::to_string(moves.size()));
}

} // namespace client
