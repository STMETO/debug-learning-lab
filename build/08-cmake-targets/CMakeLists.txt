cmake_minimum_required(VERSION 3.20)

project(cmake_targets LANGUAGES C)

add_library(format STATIC src/format.c)

target_include_directories(format
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(format PUBLIC c_std_11)
target_compile_options(format PRIVATE -Wall -Wextra -Wpedantic)

add_executable(target_demo src/main.c)
target_link_libraries(target_demo PRIVATE format)
