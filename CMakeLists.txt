cmake_minimum_required(VERSION 3.16)
project(blink C CXX)

# On HPC systems where CC/CXX are already MPI wrappers (mpicc/mpicxx),
# FindMPI's link-test often fails because LD_LIBRARY_PATH isn't set up for
# CMake's subprocess.  Pass -DBLINK_MPI_WRAPPER=ON to skip FindMPI and create
# stub INTERFACE targets instead — the wrapper already injects all flags.
option(BLINK_MPI_WRAPPER
    "Assume CC/CXX are MPI compiler wrappers; skip find_package(MPI)" OFF)

if(BLINK_MPI_WRAPPER)
    message(STATUS "BLINK_MPI_WRAPPER=ON — querying '${CMAKE_C_COMPILER}' for MPI flags")
    execute_process(COMMAND "${CMAKE_C_COMPILER}" --showme:incdirs
                    OUTPUT_VARIABLE _mpi_incdirs OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND "${CMAKE_C_COMPILER}" --showme:libdirs
                    OUTPUT_VARIABLE _mpi_libdirs OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND "${CMAKE_C_COMPILER}" --showme:libs
                    OUTPUT_VARIABLE _mpi_libs    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string(REPLACE " " ";" _mpi_incdirs "${_mpi_incdirs}")
    string(REPLACE " " ";" _mpi_libdirs "${_mpi_libdirs}")
    string(REPLACE " " ";" _mpi_libs    "${_mpi_libs}")
    set(_mpi_link_flags "")
    foreach(_d ${_mpi_libdirs})
        list(APPEND _mpi_link_flags "-L${_d}")
    endforeach()
    foreach(_l ${_mpi_libs})
        list(APPEND _mpi_link_flags "-l${_l}")
    endforeach()
    message(STATUS "  MPI includes : ${_mpi_incdirs}")
    message(STATUS "  MPI link     : ${_mpi_link_flags}")
    foreach(_tgt MPI::MPI_C MPI::MPI_CXX)
        if(NOT TARGET ${_tgt})
            add_library(${_tgt} INTERFACE IMPORTED)
        endif()
        set_target_properties(${_tgt} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_mpi_incdirs}"
            INTERFACE_LINK_LIBRARIES     "${_mpi_link_flags}")
    endforeach()
else()
    find_package(MPI REQUIRED COMPONENTS C CXX)
endif()

# All binaries land in <build>/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Default to Release so -O3 is active unless the user overrides
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BLINK_COVERAGE "Build with gcov coverage instrumentation (-O0 --coverage)" OFF)
if(BLINK_COVERAGE)
    # -O0 for accurate line attribution; atomic counter updates so the multiple
    # concurrent MPI ranks of a single benchmark binary don't corrupt the shared
    # .gcda files when they dump coverage on exit.
    add_compile_options(-O0 -g --coverage -fprofile-update=atomic)
    add_link_options(--coverage)
else()
    add_compile_options(-O3)
endif()
add_compile_definitions(_GNU_SOURCE)

add_subdirectory(src)

option(BLINK_TESTS "Build correctness tests (requires GTest)" ON)

if(BLINK_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found via find_package — fetching with FetchContent")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        if(NOT TARGET GTest::GTest)
            add_library(GTest::GTest ALIAS gtest)
            add_library(GTest::Main  ALIAS gtest_main)
        endif()
        set(GTest_FOUND TRUE)
    endif()

    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(tests)
        message(STATUS "GTest ready — correctness tests enabled (run: ctest --test-dir build)")
    else()
        message(WARNING "GTest unavailable — correctness tests skipped (pass -DBLINK_TESTS=OFF to silence)")
    endif()
else()
    message(STATUS "BLINK_TESTS=OFF — correctness tests skipped")
endif()
