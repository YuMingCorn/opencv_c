cmake_minimum_required(VERSION 3.21)

project(opencv_c LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Freestanding targets (bare-metal toolchains use CMAKE_SYSTEM_NAME Generic)
# default to the portable core only.
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(_improc_bare ON)
else()
    set(_improc_bare OFF)
endif()

option(IMPROC_CORE_ONLY "Build only improc_core (no IO, demo, bench, tests)" ${_improc_bare})
option(IMPROC_NO_STDIO "Core never includes <stdio.h>" ${_improc_bare})
option(IMPROC_NO_MALLOC "No default allocator; alloc_mat/free_mat not compiled" OFF)
option(IMPROC_ENABLE_FLOAT "Compile the float sources (IMPROC_HAS_FLOAT=1)" ON)
option(IMPROC_ASSERT_TERSE "ASSERT passes NULL, NULL, line to the handler" OFF)
option(IMPROC_WERROR "Treat compiler warnings as errors" OFF)
set(IMPROC_KODAK_DIR "${CMAKE_SOURCE_DIR}/datasets/kodak"
    CACHE PATH "Directory holding kodim01.png ... kodim24.png")

function(improc_warnings target)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    if(IMPROC_WERROR)
        target_compile_options(${target} PRIVATE -Werror)
    endif()
endfunction()

# Kodak regression test: tests/kodak_ops_<module>.c against
# tests/golden/kodak_<module>.txt. Skipped (exit 77) when the dataset is
# absent. The IMPROC_KODAK_DIR environment variable overrides the directory at
# run time.
function(add_kodak_test module)
    set(name test_kodak_${module})
    add_executable(${name} tests/kodak_main.c tests/kodak_ops_${module}.c)
    improc_warnings(${name})
    target_include_directories(${name} PRIVATE tests)
    target_compile_definitions(${name} PRIVATE KODAK_MODULE="${module}")
    target_link_libraries(${name} PRIVATE improc)
    add_test(NAME ${name}
             COMMAND ${name} --dir ${IMPROC_KODAK_DIR}
                     --manifest ${CMAKE_SOURCE_DIR}/tests/golden/kodak_${module}.txt
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77)
endfunction()

# ---- improc_core: portable, no OS dependency -------------------------------
set(IMPROC_CORE_SOURCES
    core/color.c
    core/pixel.c
    core/filter.c
    core/geometry.c
    core/mat.c
    core/utils.c
    core/morph.c
    core/hist.c
    # lane-append: core sources
)

# Sources that need float support (not compiled when IMPROC_ENABLE_FLOAT=OFF).
set(IMPROC_FLOAT_SOURCES
    core/convert_f32.c
    # lane-append: float sources
)
# Float sources: no fused multiply-add (bit-identical across targets) and no
# implicit float -> double promotion.
set_source_files_properties(core/convert_f32.c PROPERTIES
    COMPILE_OPTIONS "-ffp-contract=off;-Wdouble-promotion")

add_library(improc_core STATIC ${IMPROC_CORE_SOURCES})
if(IMPROC_ENABLE_FLOAT AND IMPROC_FLOAT_SOURCES)
    target_sources(improc_core PRIVATE ${IMPROC_FLOAT_SOURCES})
endif()
target_include_directories(improc_core PUBLIC core)
improc_warnings(improc_core)
if(IMPROC_ENABLE_FLOAT)
    target_compile_definitions(improc_core PUBLIC IMPROC_HAS_FLOAT=1)
    if(UNIX)
        target_link_libraries(improc_core PUBLIC m)
    endif()
else()
    target_compile_definitions(improc_core PUBLIC IMPROC_HAS_FLOAT=0)
endif()
# Hosted code (mat_io.c, tests, demo) needs the default allocator.
if(IMPROC_NO_MALLOC AND NOT IMPROC_CORE_ONLY)
    message(FATAL_ERROR
        "IMPROC_NO_MALLOC=ON requires IMPROC_CORE_ONLY=ON: the hosted image "
        "IO, demo, benchmark and tests allocate with alloc_mat/free_mat.")
endif()
if(IMPROC_NO_STDIO)
    target_compile_definitions(improc_core PUBLIC IMPROC_NO_STDIO=1)
endif()
if(IMPROC_NO_MALLOC)
    target_compile_definitions(improc_core PUBLIC IMPROC_NO_MALLOC=1)
endif()
if(IMPROC_ASSERT_TERSE)
    target_compile_definitions(improc_core PUBLIC IMPROC_ASSERT_TERSE=1)
endif()
if(IMPROC_CORE_ONLY)
    target_compile_definitions(improc_core PUBLIC IMPROC_CORE_ONLY=1)
    return()
endif()

# ---- improc: core plus hosted image IO -------------------------------------
find_package(Threads REQUIRED)

add_library(improc STATIC core/mat_io.c)
improc_warnings(improc)
target_link_libraries(improc PUBLIC improc_core Threads::Threads)
# stb_image / stb_image_write call pow() even when the float sources are off.
if(UNIX)
    target_link_libraries(improc PUBLIC m)
endif()

add_executable(demo main.c)
improc_warnings(demo)
target_link_libraries(demo PRIVATE improc)

add_executable(bench_gray benchmarks/bench_gray.c)
improc_warnings(bench_gray)
target_link_libraries(bench_gray PRIVATE improc)

enable_testing()

function(add_improc_test name)
    add_executable(${name} tests/${name}.c)
    improc_warnings(${name})
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
add_improc_test(test_hooks)
add_improc_test(test_morph)
add_improc_test(test_hist)
add_improc_test(test_f32)
add_improc_test(test_scratch)
add_improc_test(test_arduino_synthetic)
# lane-append: tests

add_kodak_test(phase1)
add_kodak_test(morph)
add_kodak_test(hist)
add_kodak_test(color2)
add_kodak_test(area)
if(IMPROC_ENABLE_FLOAT)
    add_kodak_test(f32)
endif()
# lane-append: kodak tests

add_test(NAME demo_smoke COMMAND demo
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
