add_executable(server
    main.cpp
    src/console.cpp
    src/game.cpp
    src/server.cpp
    src/stats.cpp
)

target_include_directories(server PRIVATE include)
target_link_libraries(server PRIVATE common)
