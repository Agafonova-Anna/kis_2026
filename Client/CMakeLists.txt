add_executable(client
    main.cpp
    src/client.cpp
    src/console.cpp
)

target_include_directories(client PRIVATE include)
target_link_libraries(client PRIVATE common)
