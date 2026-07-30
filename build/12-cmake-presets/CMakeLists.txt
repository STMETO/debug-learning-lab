cmake_minimum_required(VERSION 3.21)

project(cmake_presets LANGUAGES C)

option(ENABLE_ASAN "Build with AddressSanitizer" OFF)

add_executable(preset_demo main.c)
target_compile_features(preset_demo PRIVATE c_std_11)
target_compile_options(preset_demo PRIVATE -Wall -Wextra -Wpedantic)

if(ENABLE_ASAN)
    target_compile_definitions(preset_demo PRIVATE ENABLE_ASAN=1)
    target_compile_options(preset_demo PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(preset_demo PRIVATE -fsanitize=address)
endif()
