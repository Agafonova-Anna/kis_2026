#include <cstdint>
#include <exception>
#include <format>
#include <iostream>

#include "common/protocol.hpp"
#include "common/tcp.hpp"
#include "server/server.hpp"

int main(int argc, char* argv[]) {
    common::ignore_sigpipe();

    std::uint16_t port = common::kDefaultPort;
    if (argc > 1) {
        const auto parsed = common::parse_int(argv[1]);
        if (argc > 2 || !parsed || *parsed < 1 || *parsed > 65535) {
            std::cerr << std::format("Использование: {} [порт]", argv[0]) << std::endl;
            return 1;
        }
        port = static_cast<std::uint16_t>(*parsed);
    }

    try {
        server::Server(port).run();
    } catch (const std::exception& e) {
        std::cerr << std::format("Ошибка: {}", e.what()) << std::endl;
        return 1;
    }
    return 0;
}
