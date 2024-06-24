/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for UFA JSRON-RPC server.                                      */
/* Listens for jsonrpc requests and calls functions according to the method   */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#ifndef UFA_JSONRPC_SERVER_H_
#define UFA_JSONRPC_SERVER_H_

#include <stdbool.h>
#include "util/error.h"

#define SOCKET_PATH "/tmp/ufarpc_unix_sock.server"
#define SOCKET_CLIENT_PATH "/tmp/ufarpc_unix_sock.client"

typedef struct ufa_jsonrpc_server ufa_jsonrpc_server_t;


/**
 * Create a new ufa_jsonrpc_server_t object.
 *
 * @return Newly-allocated object
 */
ufa_jsonrpc_server_t *ufa_jsonrpc_server_new();

/**
 * Start JSON-RPC Server.
 *
 * @param server JSON-RPC Server object
 * @param error
 */
void ufa_jsonrpc_server_start(ufa_jsonrpc_server_t *server,
			      struct ufa_error **error);

/**
 * Stop JSON-RPC Server.
 *
 * @param server JSON-RPC Server object
 * @param error
 */
void ufa_jsonrpc_server_stop(ufa_jsonrpc_server_t *server,
			     struct ufa_error **error);


/**
 * Free resources of JSON-RPC Server
 *
 * @param server
 */
void ufa_jsonrpc_server_free(ufa_jsonrpc_server_t *server);

#endif // UFA_JSONRPC_SERVER_H_
