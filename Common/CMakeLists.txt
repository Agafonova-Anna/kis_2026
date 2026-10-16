add_library(common STATIC
    src/board.cpp
    src/connection.cpp
    src/line_buffer.cpp
    src/mark.cpp
    src/protocol.cpp
    src/socket.cpp
    src/tcp.cpp
)

target_include_directories(common PUBLIC include)
