cmake_minimum_required(VERSION 3.20)

project(cmake_basics LANGUAGES C)

add_executable(hello_cmake main.c)

target_compile_features(hello_cmake PRIVATE c_std_11)
target_compile_options(hello_cmake PRIVATE -Wall -Wextra -Wpedantic)
