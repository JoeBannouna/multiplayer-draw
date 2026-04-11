cmake_minimum_required(VERSION 3.16)
project(multiplayer-draw C)

# Find SDL2 (installed via apt)
find_package(SDL2 REQUIRED)

# Collect source files
set(CLIENT_SOURCES
    client/main.c
)
set(SERVER_SOURCES
    server/app.c
)

# Define the executable target
add_executable(client ${CLIENT_SOURCES})
add_executable(server ${SERVER_SOURCES})

# C standard
set_target_properties(client PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
set_target_properties(server PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)

# Compiler warnings and optimizations
target_compile_options(client PRIVATE
    -Wall -Wextra -Werror -Wconversion
    -O2
)
target_compile_options(server PRIVATE
    -Wall -Wextra -Werror -Wconversion
    -O2
)

# Threading
find_package(Threads REQUIRED)

# Link SDL2 to your target (modern style)
target_link_libraries(client PRIVATE SDL2::SDL2 Threads::Threads)
target_link_libraries(server PRIVATE Threads::Threads)

# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
