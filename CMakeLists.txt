# ========================================================================== #
# Copyright (c) 2023 Henrique Teófilo                                        #
# All rights reserved.                                                       #
#                                                                            #
# This file is part of UFA project.                                          #
# For the terms of usage and distribution, please see COPYING file.          #
# ========================================================================== #

# Basic project settings
cmake_minimum_required(VERSION 3.22)
project(ufa C)
set(CMAKE_C_STANDARD 99)
add_compile_definitions(_GNU_SOURCE)

# Modules for CMake
include(FindPkgConfig)

# Choosing directory of generated binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Third party libraries
pkg_check_modules(FUSE REQUIRED fuse3)
pkg_check_modules(SQLITE REQUIRED sqlite3)

# POSIX Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Root of include directories
include_directories(src/)

# Adding dir of ufa modules
add_subdirectory(src/util)
add_subdirectory(src/core)
add_subdirectory(src/tools)
