cmake_minimum_required(VERSION 3.9)
project(ssb C)

# Hey, I like to declare my variables inside my for loops!
set(CMAKE_C_STANDARD 99)

# Get last commit hash
execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get last commit timestamp
execute_process(
        COMMAND git log -1 --format=%ci
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_TIMESTAMP
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add Git info as preprocessor definitions
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
add_definitions(-DGIT_COMMIT_TIMESTAMP="${GIT_COMMIT_TIMESTAMP}")

# Link Winsock libraries
link_libraries(ws2_32 wsock32)

add_executable(ssb main.c server.c server.h session.c session.h state.c state.h terminal.c terminal.h config.h game.c game.h)
