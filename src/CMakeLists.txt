# Auto-discover every .c and .cpp file under src/ and build one executable per file.
# CONFIGURE_DEPENDS re-triggers configure when files are added or removed.
# common.h is in this directory (src/) and is found via the include path below.
# All executable names are derived from the source filename stem, e.g.
#   alltoall/alltoall_nb.c  ->  bin/alltoall_nb

file(GLOB_RECURSE c_srcs   CONFIGURE_DEPENDS "*.c")
file(GLOB_RECURSE cpp_srcs CONFIGURE_DEPENDS "*.cpp")

foreach(src IN LISTS c_srcs cpp_srcs)
    get_filename_component(name ${src} NAME_WE)
    get_filename_component(ext  ${src} EXT)

    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if(ext STREQUAL ".c")
        target_link_libraries(${name} PRIVATE MPI::MPI_C m)
    else()
        target_link_libraries(${name} PRIVATE MPI::MPI_CXX m)
    endif()
endforeach()
