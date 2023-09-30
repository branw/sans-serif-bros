cmake_minimum_required(VERSION 3.9)

# Use submodule vcpkg if no other path is provided
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ext/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

project(ssb C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Main target
set(SOURCES
        src/server.c src/session.c src/state.c src/terminal.c src/game.c
        src/util.c src/canvas.c src/db.c src/screen.c src/log.c
        src/screens/title.c src/screens/levels.c src/screens/game.c)

add_executable(ssb src/main.c ${SOURCES})
target_include_directories(ssb PRIVATE src ext/baro)

target_compile_options(ssb PRIVATE -fsanitize=address,undefined -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=)
target_link_options(ssb PRIVATE -fsanitize=address,undefined)

target_link_libraries(ssb PRIVATE unofficial::sqlite3::sqlite3)

# Unit tests
add_executable(test-ssb ext/baro/baro.c ${SOURCES})
target_include_directories(test-ssb PRIVATE src ext/baro)
target_compile_definitions(test-ssb PRIVATE BARO_ENABLE)

target_compile_options(test-ssb PRIVATE -fsanitize=address,undefined)
target_link_options(test-ssb PRIVATE -fsanitize=address,undefined)

target_link_libraries(test-ssb PRIVATE unofficial::sqlite3::sqlite3)

# Fuzzer targets
if(DEFINED ENV{GITHUB_ACTIONS})
    message(STATUS "Skipping fuzzer target build because we are in GitHub Actions")
else()
    add_executable(fuzz-ssb-terminal-parse src/fuzzers/fuzz_terminal_parse.cpp ${SOURCES})
    target_include_directories(fuzz-ssb-terminal-parse PRIVATE src ext/baro)

    target_compile_options(fuzz-ssb-terminal-parse PRIVATE -g -O0 -fsanitize=fuzzer,address)
    target_link_libraries(fuzz-ssb-terminal-parse PRIVATE -fsanitize=fuzzer,address)

    target_link_libraries(fuzz-ssb-terminal-parse PRIVATE unofficial::sqlite3::sqlite3)
endif()

enable_testing()
add_test(ssb test-ssb)
