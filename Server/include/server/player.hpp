#pragma once

#include <string>
#include <vector>

namespace server {

struct Player {
    std::string name;
    int session = -1;
    std::vector<int> games;
    int wins = 0;
    int losses = 0;
    int draws = 0;

    bool online() const { return session >= 0; }
    int played() const { return wins + losses + draws; }
    double win_rate() const { return played() > 0 ? static_cast<double>(wins) / played() : 0.0; }
};

} // namespace server
