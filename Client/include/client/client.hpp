#pragma once

#include <poll.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "common/board.hpp"
#include "common/connection.hpp"
#include "common/line_buffer.hpp"
#include "common/mark.hpp"

namespace client {

class Client {
public:
    Client(std::string host, std::string port, std::string name);
    void run();

private:
    using Args = std::vector<std::string>;

    struct Game {
        common::Mark mark;
        common::Board board;
    };

private:
    std::vector<pollfd> collect_fds() const;
    void poll_and_dispatch(std::vector<pollfd>& fds);
    bool connect();
    void on_server_readable();
    void connection_lost();
    bool send(const std::string& line);
    void on_server_line(const std::string& line);
    void on_invite(const Args& parts);
    void on_game(const Args& parts);

private:
    void on_console_readable();
    void on_console_line(const std::string& line);
    void cmd_answer(const Args& args, bool accepted);
    void cmd_move(const Args& args);
    void cmd_history(const Args& args) const;
    std::optional<int> current_game() const;
    void print_state(int id, const Game& game) const;

private:
    std::string host_;
    std::string port_;
    std::string name_;
    std::optional<common::Connection> connection_;
    common::LineBuffer console_;
    std::set<int> invites_;
    std::map<int, Game> games_;
    bool running_ = true;
};

} // namespace client
