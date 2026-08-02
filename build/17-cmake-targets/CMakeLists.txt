cmake_minimum_required(VERSION 3.21)
project(cmake_targets LANGUAGES C)

add_executable(target_demo)
target_sources(target_demo PRIVATE src/main.c src/calc.c)
target_include_directories(target_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(target_demo PRIVATE c_std_11)
target_compile_options(target_demo PRIVATE -Wall -Wextra -Wpedantic)
