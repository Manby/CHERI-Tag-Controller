cmake_minimum_required(VERSION 3.10)

project(CHERI-Tag-Controller)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
        src/main.cpp
)

include_directories(
        include
)

add_executable(cheri-tag-controller ${SOURCES})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
