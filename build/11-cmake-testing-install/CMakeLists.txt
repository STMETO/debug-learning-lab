cmake_minimum_required(VERSION 3.20)

project(cmake_testing_install VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)
include(CTest)

add_library(textutil STATIC src/textutil.c)
add_library(textutil::textutil ALIAS textutil)

target_include_directories(textutil
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(textutil PUBLIC c_std_11)
target_compile_options(textutil PRIVATE -Wall -Wextra -Wpedantic)

add_executable(word_count src/main.c)
target_link_libraries(word_count PRIVATE textutil::textutil)

add_executable(test_textutil tests/test_textutil.c)
target_link_libraries(test_textutil PRIVATE textutil::textutil)
add_test(NAME textutil.unit COMMAND test_textutil)

install(TARGETS textutil word_count
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES include/textutil.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
