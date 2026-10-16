#include "server/stats.hpp"

#include <cmath>

namespace server {

std::string envy_message(const Player& target, const std::map<std::string, Player>& players) {
    int others = 0;
    int better = 0;
    for (const auto& [name, player] : players) {
        if (name == target.name || player.played() == 0) {
            continue;
        }
        ++others;
        if (player.win_rate() > target.win_rate()) {
            ++better;
        }
    }

    std::string text = target.played() == 0
                           ? "Вы ещё ни разу не играли с Мобиусом."
                           : "Ваши победы: " + std::to_string(target.wins) + " из " +
                                 std::to_string(target.played()) + " (" +
                                 std::to_string(percent(target.win_rate())) + "%).";
    if (others == 0) {
        return text + " Сравнить вас пока не с кем, но другие варианты уже на подходе.";
    }
    const int worse = percent(static_cast<double>(better) / others);
    if (worse == 0) {
        return text + " Пока вы не хуже ни одной из своих версий. Пока.";
    }
    text += " Вы хуже " + std::to_string(worse) + "% своих версий.";
    return text + (worse >= 50 ? " Вы вообще стараетесь?" : " Настоящий Локи так бы не играл.");
}

int percent(double share) {
    return static_cast<int>(std::lround(share * 100));
}

} // namespace server
