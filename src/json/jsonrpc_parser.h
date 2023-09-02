/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for a simple JRON-RPC parser                                   */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_JSONRPC_PARSER_H_
#define UFA_JSONRPC_PARSER_H_

#include "util/hashtable.h"
#include "util/jsmn.h"

/** Invalid JSON was received by the server.
 * An error occurred on the server while parsing the JSON text. */
#define JSONRPC_PARSE_ERROR -32700

/** The JSON sent is not a valid Request object. */
#define JSONRPC_INVALID_REQUEST -32600

/** Method does not exist / is not available. */
#define JSONRPC_METHOD_NOT_FOUND -32601

/** Invalid method parameter(s). */
#define JSONRPC_INVALID_PARAMS -32602

/** Internal JSON-RPC error. */
#define JSONRPC_INTERNAL_ERROR -32603


/**
 * Struct that represents a JSON-RPC request and response
 */
struct ufa_jsonrpc {
	char *method;
	char *id;
	ufa_hashtable_t *params;
	ufa_hashtable_t *result;
	ufa_hashtable_t *error;
};

/**
 * Possible returns of parse function
 */
enum ufa_parser_result {
	UFA_JSON_OK         = 0,
	UFA_JSON_NOMEM      = JSMN_ERROR_NOMEM,
	UFA_JSON_INVAL      = JSMN_ERROR_INVAL,
	UFA_JSON_PART       = JSMN_ERROR_PART,
	UFA_JSONRPC_INVALID = -5000,
};

/**
 * Parse function for a JSON-RPC request or response.
 * @param json JSON as string
 * @param jsonrpc Pointer to Pointer to a struct that will be created and filled
 *                as result of parsing
 * @return ufa_parser_result value enum
 */
enum ufa_parser_result ufa_jsonrpc_parse(const char *json,
					 struct ufa_jsonrpc **jsonrpc);

/**
 * Free all resources allocated for a ufa_jsonrpc (including all child objects)
 * @param p Pointer to a struct ufa_jsonrpc
 */
void ufa_jsonrpc_free(struct ufa_jsonrpc *p);

#endif // UFA_JSONRPC_PARSER_H_
