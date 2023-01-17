# ========================================================================== #
# Copyright (c) 2023 Henrique Teófilo                                        #
# All rights reserved.                                                       #
#                                                                            #
# This file is part of UFA project.                                          #
# For the terms of usage and distribution, please see COPYING file.          #
# ========================================================================== #


find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})


add_executable(check_repo_sqlite check_repo_sqlite.c)
target_link_libraries(check_repo_sqlite ufa-core ${CHECK_LIBRARIES} Threads::Threads)


add_test(NAME check_repo_sqlite COMMAND check_repo_sqlite)