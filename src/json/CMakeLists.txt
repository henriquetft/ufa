# ============================================================================ #
# Copyright (c) 2023 Henrique Teófilo                                          #
# All rights reserved.                                                         #
#                                                                              #
# This file is part of UFA project.                                            #
# For the terms of usage and distribution, please see COPYING file.            #
# ============================================================================ #

# JSON-RPC api (lib)
add_library(ufa-jsonrpc-api
        jsonrpc_api.c
        jsonrpc_parser.c)
target_link_libraries(ufa-jsonrpc-api
        ufa-core
        ufa-util)

# JSON-RPC server (lib)
add_library(ufa-jsonrpc-server
        jsonrpc_server.c)
target_link_libraries(ufa-jsonrpc-server
        ufa-core
        ufa-jsonrpc-api)

# JSON-RPC parser (lib)
add_library(ufa-jsonrpc-parser
        jsonrpc_parser.c)
target_link_libraries(ufa-jsonrpc-parser
        ufa-util)

