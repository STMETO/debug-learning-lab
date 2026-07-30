cmake_minimum_required(VERSION 3.20)

project(cmake_configurations LANGUAGES C)

option(ENABLE_ASAN "Build with AddressSanitizer" OFF)

add_executable(config_demo buggy.c)
target_compile_features(config_demo PRIVATE c_std_11)
target_compile_options(config_demo PRIVATE -Wall -Wextra -Wpedantic)

if(ENABLE_ASAN)
    target_compile_options(config_demo PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(config_demo PRIVATE -fsanitize=address)
endif()
