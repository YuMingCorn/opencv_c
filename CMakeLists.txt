# CMakeLists.txt

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.11)

# Define Project Name
project(MyProject C)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -ftree-vectorize")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the executable name as a variable to avoid repetition
set(EXECUTABLE_NAME main)

# Add all .c files in the src folder to sources
file(GLOB c_file "*.c" "core/*.c")
file(GLOB header_file "*.h" "core/*.h")

find_package(Threads REQUIRED)


# Create the executable
add_executable(${EXECUTABLE_NAME} ${c_file})

# Set the include path
target_include_directories(${EXECUTABLE_NAME} PUBLIC "core")

# Set debug options for compilation
target_compile_options(${EXECUTABLE_NAME} PRIVATE -g)  # Enable debug information

target_link_libraries(${EXECUTABLE_NAME} PRIVATE Threads::Threads)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE m)
# Optionally, set the default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# This allows you to control CMake options for different build types
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_MODE")  # Additional flags for debug mode
