# ============================================================================ #
# Copyright (c) 2023 Henrique Teófilo                                          #
# All rights reserved.                                                         #
#                                                                              #
# This file is part of UFA project.                                            #
# For the terms of usage and distribution, please see COPYING file.            #
# ============================================================================ #


# Adding includes for third party libraries
include_directories(${FUSE_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS})

# UFA core (lib)
add_library(ufa-core
        repo_sqlite.c
        data.c
        config.c)
target_link_libraries(ufa-core
        ${SQLITE_LDFLAGS}
        ufa-util)


# Adding executable ufafs
add_executable(ufafs ufafs.c repo_sqlite.c)
target_link_libraries(ufafs
        ${FUSE_LDFLAGS}
        ${SQLITE_LDFLAGS}
        ufa-util)

# Adding executable ufad
add_executable(ufad
        ufad.c
        monitor_inotify.c)
target_link_libraries(ufad ufa-core ufa-jsonrpc-server Threads::Threads)

