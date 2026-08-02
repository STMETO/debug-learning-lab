cmake_minimum_required(VERSION 3.21)
project(cmake_language LANGUAGES C)

set(APP_NAME language_demo)
set(APP_SOURCES main.c)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)

message(STATUS "APP_NAME=${APP_NAME}")
message(STATUS "ENABLE_WARNINGS=${ENABLE_WARNINGS}")

add_executable(${APP_NAME} ${APP_SOURCES})

if(ENABLE_WARNINGS)
    target_compile_options(${APP_NAME} PRIVATE -Wall -Wextra)
endif()
