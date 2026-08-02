cmake_minimum_required(VERSION 3.21)
project(cmake_test_install LANGUAGES C)
include(GNUInstallDirs)
include(CTest)

add_library(calc STATIC calc.c)
target_include_directories(calc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_executable(calc_app main.c)
target_link_libraries(calc_app PRIVATE calc)
add_executable(test_calc test_calc.c)
target_link_libraries(test_calc PRIVATE calc)
add_test(NAME calc.unit COMMAND test_calc)

install(TARGETS calc calc_app
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES calc.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
