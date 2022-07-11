# Basic project settings
cmake_minimum_required(VERSION 3.22)
project(ufa C)
set(CMAKE_C_STANDARD 99)


include(FindPkgConfig)


# Choosing directory of generated binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Third party libraries
pkg_check_modules(FUSE REQUIRED fuse3)
pkg_check_modules(SQLITE REQUIRED sqlite3)

# Root of include directories
include_directories(src/)

# Adding dir of ufa modules
add_subdirectory(src/util)
add_subdirectory(src/core)
add_subdirectory(src/tools)







