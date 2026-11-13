cmake_minimum_required(VERSION 3.16)

project(opencv_c LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

add_library(improc STATIC
    core/color.c
    core/pixel.c
    core/filter.c
    core/geometry.c
    core/mat.c
    core/utils.c)
target_include_directories(improc PUBLIC core)
target_compile_options(improc PRIVATE -Wall -Wextra)
target_link_libraries(improc PUBLIC Threads::Threads)
if(UNIX)
    target_link_libraries(improc PUBLIC m)
endif()

add_executable(demo main.c)
target_compile_options(demo PRIVATE -Wall -Wextra)
target_link_libraries(demo PRIVATE improc)

add_executable(bench_gray benchmarks/bench_gray.c)
target_compile_options(bench_gray PRIVATE -Wall -Wextra)
target_link_libraries(bench_gray PRIVATE improc)

enable_testing()

function(add_improc_test name)
    add_executable(${name} tests/${name}.c)
    target_compile_options(${name} PRIVATE -Wall -Wextra)
    target_link_libraries(${name} PRIVATE improc)
    add_test(NAME ${name} COMMAND ${name}
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77)
endfunction()

add_improc_test(test_mat)
add_improc_test(test_color)
add_improc_test(test_pixel)
add_improc_test(test_filter)
add_improc_test(test_geometry)
add_improc_test(test_io)

add_test(NAME demo_smoke COMMAND demo
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
