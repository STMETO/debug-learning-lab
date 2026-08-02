cmake_minimum_required(VERSION 3.21)
project(cmake_libraries LANGUAGES C)

add_library(math_static STATIC src/mathlib.c)
add_library(math_shared SHARED src/mathlib.c)

foreach(target IN ITEMS math_static math_shared)
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME mathlab)
endforeach()

add_executable(app_static src/main.c)
target_link_libraries(app_static PRIVATE math_static)
add_executable(app_shared src/main.c)
target_link_libraries(app_shared PRIVATE math_shared)
