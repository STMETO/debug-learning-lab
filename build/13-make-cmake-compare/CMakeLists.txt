cmake_minimum_required(VERSION 3.20)

project(make_cmake_compare LANGUAGES C)

add_executable(compare_demo
    src/main.c
    src/stats.c
)

target_include_directories(compare_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(compare_demo PRIVATE c_std_11)
target_compile_options(compare_demo PRIVATE -g -Wall -Wextra -Wpedantic)
