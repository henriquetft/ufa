# ============================================================================ #
# Copyright (c) 2023 Henrique Teófilo                                          #
# All rights reserved.                                                         #
#                                                                              #
# This file is part of UFA project.                                            #
# For the terms of usage and distribution, please see COPYING file.            #
# ============================================================================ #


find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})

add_executable(check_string check_string.c)
target_link_libraries(check_string ufa-util ${CHECK_LIBRARIES} Threads::Threads)

add_executable(check_list check_list.c)
target_link_libraries(check_list ufa-util ${CHECK_LIBRARIES} Threads::Threads)

add_executable(check_hashtable check_hashtable.c)
target_link_libraries(check_hashtable ufa-util ${CHECK_LIBRARIES} Threads::Threads)

add_executable(check_config check_config.c)
target_link_libraries(check_config ufa-core ${CHECK_LIBRARIES} Threads::Threads)

add_executable(check_parser check_parser.c)
target_link_libraries(check_parser ufa-jsonrpc-parser ${CHECK_LIBRARIES} Threads::Threads)

add_executable(check_repo_sqlite check_repo_sqlite.c)
target_link_libraries(check_repo_sqlite ufa-core ${CHECK_LIBRARIES} Threads::Threads)

add_executable(check_jsonrpc_api check_jsonrpc_api.c)
target_link_libraries(check_jsonrpc_api ufa-jsonrpc-api ufa-jsonrpc-server ${CHECK_LIBRARIES} Threads::Threads)


# These tests will be run by CTest
add_test(NAME check_string COMMAND check_string)
add_test(NAME check_list COMMAND check_list)
add_test(NAME check_hashtable COMMAND check_hashtable)
add_test(NAME check_config COMMAND check_config)
add_test(NAME check_parser COMMAND check_parser)
add_test(NAME check_repo_sqlite COMMAND check_repo_sqlite)
add_test(NAME check_jsonrpc_api COMMAND check_jsonrpc_api)
