cmake_minimum_required(VERSION 3.10)

project(CHERI-Tag-Controller)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
        src/main.cpp
)

include_directories(
        include
)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(cheri-tag-controller ${SOURCES})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

enable_testing()

add_executable(
        baseline_controller_test
        test/test-BaselineController.cpp
)

add_executable(
        morello_controller_test
        test/test-MorelloController.cpp
)

add_executable(
        etm_controller_test
        test/test-ETMController.cpp
)

target_link_libraries(
        baseline_controller_test
        GTest::gtest_main
)

target_link_libraries(
        morello_controller_test
        GTest::gtest_main
)

target_link_libraries(
        etm_controller_test
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(baseline_controller_test)
gtest_discover_tests(morello_controller_test)
gtest_discover_tests(etm_controller_test)
