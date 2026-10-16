#pragma once

#include <map>
#include <string>

#include "server/player.hpp"

namespace server {

std::string envy_message(const Player& target, const std::map<std::string, Player>& players);

int percent(double share);

} // namespace server
