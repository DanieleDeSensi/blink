cmake_minimum_required(VERSION 3.16)
project(blink C CXX)

find_package(MPI REQUIRED COMPONENTS C CXX)

# All binaries land in <build>/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Default to Release so -O3 is active unless the user overrides
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-O3)
add_compile_definitions(_GNU_SOURCE)

add_subdirectory(src)
