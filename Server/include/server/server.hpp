#pragma once

#include <poll.h>

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "common/connection.hpp"
#include "common/line_buffer.hpp"
#include "common/socket.hpp"
#include "server/game.hpp"
#include "server/player.hpp"

namespace server {

class Server {
public:
    explicit Server(std::uint16_t port);
    void run();

private:
    using Args = std::vector<std::string>;

    struct Session {
        common::Connection connection;
        std::string player;
    };

private:
    std::vector<pollfd> collect_fds() const;
    void poll_and_dispatch(std::vector<pollfd>& fds);
    void accept_session();
    void on_session_readable(int fd);
    void drop_session(int fd);
    bool handle_message(Session& session, const std::string& line);
    bool handle_hello(Session& session, const Args& args);
    void handle_answer(Player& player, const Args& args, bool accepted);
    void handle_move(Player& player, const Args& args);
    void handle_ack(Player& player, const Args& args);
    void send_to(const Player& player, const std::string& line);
    void send_invite(const Game& game);
    void send_snapshot(const Game& game);
    void finish_game(Game& game);
    Game* variant_game(const Player& player, const std::string& local_id);

private:
    void on_console_readable();
    void on_console_line(const std::string& line);
    void cmd_players() const;
    void cmd_invite(const Args& args);
    void cmd_games() const;
    void cmd_move(const Args& args);
    void cmd_show(const Args& args);
    void cmd_history(const Args& args) const;
    Game* find_game(const std::string& id);

private:
    common::Socket listener_;
    common::LineBuffer console_;
    std::map<int, Session> sessions_;
    std::map<std::string, Player> players_;
    std::deque<Game> games_;
    bool running_ = true;
};

} // namespace server
