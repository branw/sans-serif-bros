cmake_minimum_required(VERSION 3.9)
project(ssb C)

set(CMAKE_C_STANDARD 11)


set(SOURCES
        src/server.c src/session.c src/state.c src/terminal.c src/game.c
        src/util.c src/canvas.c src/db.c src/screen.c)

add_executable(ssb src/main.c ${SOURCES})
target_include_directories(ssb PRIVATE include ext/baro)

add_executable(test-ssb ext/baro/baro.c ${SOURCES})
target_include_directories(test-ssb PRIVATE include ext/baro)
target_compile_definitions(test-ssb PRIVATE BARO_ENABLE)

enable_testing()
add_test(ssb test-ssb)
