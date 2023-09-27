# ============================================================================ #
# Copyright (c) 2023 Henrique Teófilo                                          #
# All rights reserved.                                                         #
#                                                                              #
# This file is part of UFA project.                                            #
# For the terms of usage and distribution, please see COPYING file.            #
# ============================================================================ #

# Basic project settings
cmake_minimum_required(VERSION 3.22)
project(ufa C)
set(CMAKE_C_STANDARD 99)
add_compile_definitions(_GNU_SOURCE)

# Definition for JSMN
add_compile_definitions(JSMN_STATIC)

# Set up CTest
set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full")
include(CTest)

# Modules for CMake
include(FindPkgConfig)

# Dir cmake for cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Finding check for unit test
include(FindCheck)
find_package(Check REQUIRED)

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

enable_testing()

# Add ufa subdirs
add_subdirectory(src/util)
add_subdirectory(src/core)
add_subdirectory(src/tools)
add_subdirectory(src/json)
add_subdirectory(tests)

# Generate systemd service
configure_file(config/ufad.service.in config/ufad.service)

# Executables to install in bin folder
list(APPEND UFA_TARGETS_TO_INSTALL "ufad")
list(APPEND UFA_TARGETS_TO_INSTALL "ufafs")
list(APPEND UFA_TARGETS_TO_INSTALL "ufaattr")
list(APPEND UFA_TARGETS_TO_INSTALL "ufafind")
list(APPEND UFA_TARGETS_TO_INSTALL "ufatag")
list(APPEND UFA_TARGETS_TO_INSTALL "ufactl")

# Install executables
foreach (t ${UFA_TARGETS_TO_INSTALL})
    install(TARGETS ${t} DESTINATION bin)
endforeach()

# Install systemd service
install(FILES ${PROJECT_BINARY_DIR}/config/ufad.service DESTINATION $ENV{HOME}/.config/systemd/user)
