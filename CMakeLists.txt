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

if (WIN32)
# Link Winsock libraries
link_libraries(ws2_32 wsock32)
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")

add_executable(ssb
        src/main.c src/server.c src/session.c src/state.c src/terminal.c src/game.c src/util.c src/canvas.c src/db.c)
target_include_directories(ssb PUBLIC include)
