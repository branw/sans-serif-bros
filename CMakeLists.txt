cmake_minimum_required(VERSION 3.9)
project(ssb C)

set(CMAKE_C_STANDARD 99)

link_libraries(ws2_32 wsock32)
add_executable(ssb main.c)
