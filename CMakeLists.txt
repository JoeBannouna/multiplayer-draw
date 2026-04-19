cmake_minimum_required(VERSION 3.20)
project(myproject C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)  # enables gnu11 (GNU extensions)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
)
FetchContent_MakeAvailable(SDL3)

# --- Shared compiler flags ---
set(COMMON_FLAGS
    -Wall -Wextra -Werror -O2
)

# --- Sanitizer build type ---
# Use: cmake -B build -DSANITIZE=ON
option(SANITIZE "Enable AddressSanitizer and UBSan" OFF)
if(SANITIZE)
    list(APPEND COMMON_FLAGS -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# --- Client ---
add_executable(client client/main.c)
target_compile_options(client PRIVATE ${COMMON_FLAGS})
target_link_libraries(client PRIVATE SDL3::SDL3 m pthread)

# --- Server ---
add_executable(server server/app.c)
target_compile_options(server PRIVATE ${COMMON_FLAGS})
target_link_libraries(server PRIVATE m pthread)
