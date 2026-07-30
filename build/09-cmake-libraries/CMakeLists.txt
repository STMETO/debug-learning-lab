cmake_minimum_required(VERSION 3.20)

project(cmake_libraries LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(math_static STATIC src/mathlib.c)
add_library(math_shared SHARED src/mathlib.c)

foreach(math_target IN ITEMS math_static math_shared)
    target_include_directories(${math_target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(${math_target} PUBLIC c_std_11)
    target_compile_options(${math_target} PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(${math_target} PROPERTIES OUTPUT_NAME mathlab)
endforeach()

add_executable(app_static src/main.c)
target_link_libraries(app_static PRIVATE math_static)

add_executable(app_shared src/main.c)
target_link_libraries(app_shared PRIVATE math_shared)
set_target_properties(app_shared PROPERTIES BUILD_RPATH "$ORIGIN/../lib")
