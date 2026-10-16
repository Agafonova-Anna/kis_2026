#include <exception>
#include <format>
#include <iostream>
#include <string>

#include "client/client.hpp"
#include "common/protocol.hpp"
#include "common/tcp.hpp"

namespace {

std::string prompt(const std::string& question) {
    std::cout << question << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    return answer;
}

} // namespace

int main(int argc, char* argv[]) {
    common::ignore_sigpipe();
    if (argc > 3) {
        std::cerr << std::format("Использование: {} [host[:port]] [имя]", argv[0]) << std::endl;
        return 1;
    }

    const std::string address = argc > 1 ? argv[1] : prompt("Адрес сервера Мобиуса (host[:port]): ");
    const std::string name = argc > 2 ? argv[2] : prompt("Имя варианта: ");
    if (!common::is_valid_name(name)) {
        std::cerr << std::format("Имя должно быть непустым, без пробелов и не длиннее {} байт.",
                                 common::kMaxNameLength)
                  << std::endl;
        return 1;
    }

    std::string host = address;
    std::string port = std::to_string(common::kDefaultPort);
    if (const auto colon = address.rfind(':'); colon != std::string::npos) {
        host = address.substr(0, colon);
        port = address.substr(colon + 1);
    }
    if (host.empty()) {
        host = "127.0.0.1";
    }

    try {
        client::Client(host, port, name).run();
    } catch (const std::exception& e) {
        std::cerr << std::format("Ошибка: {}", e.what()) << std::endl;
        return 1;
    }
    return 0;
}
