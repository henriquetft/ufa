/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of a JRON-RPC parser (jsonrpc_parser.h)                     */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "jsonrpc_parser.h"
#include "util/hashtable.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/string.h"
#include <string.h>


/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */


#define MAX_STR_SIZE 1024
#define MAX_TOKENS   4096

/**
 * Struct that groups all the context to parsing process.
 */
struct parser_context {
	jsmntok_t *tokens;
	size_t size;
	size_t cursor;
	const char *json;
	struct ufa_jsonrpc *obj;
};


/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */


static struct ufa_jsonrpc *jsonrpc_new();
static void fill_buf_from_token_str(jsmntok_t *tok, const char *json, char *buf);
static bool read_string(struct parser_context *ctx, void **value);
static bool read_primitive(struct parser_context *ctx, void **value);
static bool read_object(struct parser_context *ctx, ufa_hashtable_t **table);
static bool read_array(struct parser_context *ctx, size_t size,
		       struct ufa_list **list);
static bool parse_param_obj(struct parser_context *ctx,
			    ufa_hashtable_t *values);
static enum ufa_parser_result parse_message(struct parser_context *ctx);


/* ========================================================================== */
/* FUNCTIONS FROM jsonrpc_parser.h                                            */
/* ========================================================================== */


enum ufa_parser_result ufa_jsonrpc_parse(const char *json,
					 struct ufa_jsonrpc **jsonrpc)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	jsmntok_t tokens[MAX_TOKENS];
	int num_tokens = 0;

	num_tokens = jsmn_parse(&parser,
				json,
				strlen(json),
				tokens,
				MAX_TOKENS);

	if (num_tokens < 0) {
		return num_tokens;
	}

	// Creating context to parser
	struct parser_context context;
	context.tokens = tokens;
	context.size   = num_tokens;
	context.json   = json;
	context.obj    = jsonrpc_new();
	context.cursor = 0;

	*jsonrpc = context.obj;

	enum ufa_parser_result result = parse_message(&context);

	ufa_debug("");
	ufa_debug("End of parsing. Cursor: %d,  Tokens: %d",
		  context.cursor,
		  context.size);

	return result;
}


void ufa_jsonrpc_free(struct ufa_jsonrpc *p)
{
	if (p != NULL) {
		ufa_free(p->method);
		ufa_free(p->id);
		ufa_hashtable_free(p->params);
		ufa_hashtable_free(p->result);
		ufa_hashtable_free(p->error);
		p->method = NULL;
		p->id = NULL;
		p->params = NULL;
		p->result = NULL;
		p->error = NULL;
		ufa_free(p);
	}
}

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static struct ufa_jsonrpc *jsonrpc_new()
{
	struct ufa_jsonrpc *obj = ufa_malloc(sizeof *obj);

	obj->method = NULL;
	obj->id     = NULL;
	obj->params = UFA_HASHTABLE_STRING();
	obj->result = UFA_HASHTABLE_STRING();
	obj->error  = UFA_HASHTABLE_STRING();

	return obj;
}

static void fill_buf_from_token_str(jsmntok_t *tok, const char *json, char *buf)
{
	buf[0] = '\0';
	strncat(buf, json + tok->start, tok->end - tok->start);
}

static bool read_string(struct parser_context *ctx, void **value)
{
	char value_as_str[MAX_STR_SIZE] = "";

	jsmntok_t *tokens = ctx->tokens;
	const char *json  = ctx->json;
	jsmntok_t *tok    = &tokens[ctx->cursor];

	ufa_return_val_if(tok->type != JSMN_STRING, false);

	ctx->cursor++;

	strncat(value_as_str, json + tok->start, tok->end - tok->start);
	*value = ufa_str_dup(value_as_str);

	return true;
}

static bool read_primitive(struct parser_context *ctx, void **value)
{
	char value_as_str[255] = "";

	jsmntok_t *tokens = ctx->tokens;
	const char *json  = ctx->json;
	jsmntok_t *tok    = &tokens[ctx->cursor];

	ufa_return_val_if(tok->type != JSMN_PRIMITIVE, false);

	value_as_str[0] = '\0';
	strncat(value_as_str, json + tok->start, tok->end - tok->start);
	ctx->cursor++;

	if (ufa_str_equals(value_as_str, "true") ||
	    ufa_str_equals(value_as_str, "false")) {
		bool val = ufa_str_equals(value_as_str, "true");
		*value = ufa_bool_dup(val);
		ufa_debug("Read primitive bool: %ld", *((bool *) *value));

	} else if (ufa_str_equals(value_as_str, "null")) {
		*value = NULL;
		ufa_debug("Read primitive null");

	} else {
		if (ufa_str_count(value_as_str, ".")) {
			double x = 0;
			ufa_return_val_ifnot(
			    ufa_str_to_double(value_as_str, &x), false);
			*value = ufa_double_dup(x);
			ufa_debug("Read primitive double: %lf",
				  *((double *) *value));
		} else {
			long x = 0;
			ufa_return_val_ifnot(ufa_str_to_long(value_as_str, &x),
					     false);
			*value = ufa_long_dup(x);
			ufa_debug("Read primitive long: %ld",
				  *((long *) *value));
		}
	}

	return true;
}

static bool read_object(struct parser_context *ctx, ufa_hashtable_t **table)
{
	ufa_debug("Reading object...");
	ufa_hashtable_t *obj = UFA_HASHTABLE_STRING();
	*table = obj;
	bool ok = parse_param_obj(ctx, obj);

	if (!ok) {
		return false;
	} else {
		return true;
	}
}

static bool read_array(struct parser_context *ctx, size_t size,
		       struct ufa_list **list)
{
	ufa_debug("Parsing items from array ...");
	struct ufa_list* sublist = NULL;
	ctx->cursor++;
	jsmntok_t *tokens = ctx->tokens;
	for (size_t x = 0; x < size; x++) {
		void *value = NULL;
		jsmntok_t *t = &tokens[ctx->cursor];
		//ufa_debug("Item from array: %d\n", t->type);

		if (t->type == JSMN_PRIMITIVE) {
			bool ok = read_primitive(ctx, &value);
			ufa_return_val_if(!ok, false);

		} else if (t->type == JSMN_STRING) {
			bool ok = read_string(ctx, &value);
			ufa_return_val_if(!ok, false);

		} else if (t->type == JSMN_OBJECT) {
			ufa_debug("Reading object inside list ...");
			ufa_hashtable_t *table = NULL;
			read_object(ctx, &table);
			value = table;

		} else if (t->type == JSMN_ARRAY) {
			ufa_debug("Reading array inside array ...");
			bool ok = read_array(ctx, t->size, &sublist);
			ufa_return_val_if(!ok, false);

		} else {
			ufa_debug("Type %d not expected here", t->type);
			return false;
		}


		if (t->type == JSMN_OBJECT) {
			*list = ufa_list_append2(
			    *list, value,
			    (ufa_list_free_fn_t) ufa_hashtable_free);
		} else if (t->type == JSMN_ARRAY) {
			*list = ufa_list_append2(
			    *list, sublist, (ufa_list_free_fn_t) ufa_list_free);
		} else {
			*list = ufa_list_append2(*list,
						 value,
						 ufa_free);
		}
	}

	return true;
}


/* ========================================================================== */
/* FUNCTIONS TO READ JSONRPC DATA                                             */
/* ========================================================================== */

static bool parse_param(struct parser_context *ctx, ufa_hashtable_t *values)
{
	char attr[MAX_STR_SIZE]         = "";
	char value_as_str[MAX_STR_SIZE] = "";

	jsmntok_t *tokens = ctx->tokens;
	const char *json  = ctx->json;
	jsmntok_t *tok_attr = &tokens[ctx->cursor];

	ufa_return_val_if(tok_attr->type != JSMN_STRING, false);

	// Reading attr name
	fill_buf_from_token_str(tok_attr, json, attr);
	ctx->cursor++;

	jsmntok_t *tok_value = &tokens[ctx->cursor];

	ufa_debug("Parsing param '%s', type: %d", attr, tok_value->type);

	if (tok_value->type == JSMN_STRING) {
		fill_buf_from_token_str(tok_value, json, value_as_str);
		ufa_debug("Saving on param table: %s=%s",
			  attr,
			  value_as_str);
		ufa_hashtable_put(values, ufa_str_dup(attr),
				  ufa_str_dup(value_as_str));
		ctx->cursor += tok_attr->size;

	} else if (tok_value->type == JSMN_ARRAY) {
		ufa_debug("Parsing param array '%s', size: %d",
			  attr, tok_value->size);
		// adding to hashtable
		struct ufa_list *param_values = ufa_hashtable_get(values, attr);

		bool ok = read_array(ctx, tok_value->size, &param_values);
		ufa_return_val_if(!ok, false);

		if (!ufa_hashtable_has_key(values, attr)) {
			if (ufa_log_is_logging(UFA_LOG_DEBUG)) {
				ufa_debug(
				    "Add params in hashtable: %s (size: %d)",
				    attr, ufa_list_size(param_values));
			}

			ufa_hashtable_put_full(values,
					    ufa_str_dup(attr),
					    param_values,
					    ufa_free,
					    (ufa_hash_free_fn_t) ufa_list_free);
		}

	} else if (tok_value->type == JSMN_PRIMITIVE) {
		void *value = NULL;
		bool b = read_primitive(ctx, &value);
		if (!b) {
			return false;
		}
		ufa_hashtable_put(values, ufa_str_dup(attr), value);

	} else if (tok_value->type == JSMN_OBJECT) {
		ufa_debug("Parsing param obj '%s', size: %d",
			  attr, tok_value->size);
		ufa_hashtable_t *obj = UFA_HASHTABLE_STRING();

		bool ok = parse_param_obj(ctx, obj);
		ufa_return_val_if(!ok, false);

		ufa_hashtable_put_full(values,
				       ufa_str_dup(attr),
				       obj,
				       ufa_free,
				       (ufa_hash_free_fn_t) ufa_hashtable_free);
		ctx->cursor += tok_attr->size;
	} else {
		ufa_debug("Type %d not expected here", tok_value->type);
		return false;
	}

	return true;
}



static bool parse_param_obj(struct parser_context *ctx, ufa_hashtable_t *values)
{
	jsmntok_t *tokens = ctx->tokens;
	ufa_return_val_if(tokens[ctx->cursor].type != JSMN_OBJECT, false);

	size_t size = tokens[ctx->cursor].size;
	ufa_debug("\tSize params: %d", size);
	ctx->cursor++;

	for (size_t i = 0; i < size; i++) {
		ufa_debug("");
		ufa_debug("Parsing param obj %d", i);
		if (!parse_param(ctx, values)) {
			ufa_debug("Parsing param obj %d returned false", i);
			return false;
		}
	}
	return true;
}


static enum ufa_parser_result parse_message(struct parser_context *ctx)
{
	jsmntok_t *tokens        = ctx->tokens;
	size_t size              = ctx->size;
	const char *json         = ctx->json;
	struct ufa_jsonrpc *obj  = ctx->obj;

	ctx->cursor = 0;

	if (tokens[ctx->cursor].type != JSMN_OBJECT) {
		goto error;
	}

	// FIXME should be returned to the caller as a enum ?
	int last_pos = tokens[ctx->cursor].end;
	if (last_pos != strlen(ctx->json)) {
		ufa_debug("More than one object");
	}

	char attr[MAX_STR_SIZE]  = "";
	char value[MAX_STR_SIZE] = "";

	// JSONRPC responses does not have method
	bool has_method = false;

	for (ctx->cursor = 1; ctx->cursor < size;) {
		jsmntok_t *tok = &tokens[ctx->cursor];
		if (tok->start >= last_pos) {
			ufa_warn("END END END!!!");
		}
		fill_buf_from_token_str(tok, json, attr);

		if (ufa_str_equals(attr, "jsonrpc")) {
			if_goto(tok->size != 1, error);

			fill_buf_from_token_str(&tokens[++ctx->cursor],
						json,
						value);
			ctx->cursor++;
			ufa_debug("JSONRPC version: '%s'", value);

		} else if (ufa_str_equals(attr, "method")) {
			if_goto(tok->size != 1, error);
			fill_buf_from_token_str(&tokens[++ctx->cursor],
						json,
						value);
			obj->method = ufa_str_dup(value);
			has_method = true;
			ctx->cursor++;
			ufa_debug("JSONRPC method: '%s'", value);

		} else if (ufa_str_equals(attr, "id")) {
			if_goto(tok->size != 1, error);
			fill_buf_from_token_str(&tokens[++ctx->cursor],
						json,
						value);
			obj->id = ufa_str_dup(value);
			ctx->cursor++;
			ufa_debug("JSONRPC id: '%s'", obj->id);

		} else if (ufa_str_equals(attr, "params")) {
			ufa_debug("JSONRPC params");
			if_goto(tok->size != 1, error);
			ctx->cursor++;
			bool ok = parse_param_obj(ctx, obj->params);
			if_goto(!ok, error);

		} else if (ufa_str_equals(attr, "result")) {
			ufa_debug("Size result: %d/%d", size, ctx->cursor);
			if_goto(tok->size != 1, error);
			ctx->cursor++;
			bool ok = parse_param_obj(ctx, obj->result);
			if_goto(!ok, error);


		} else if (ufa_str_equals(attr, "error")) {
			ufa_debug("Size error: %d/%d", size, ctx->cursor);
			if_goto(tok->size != 1, error);
			ctx->cursor++;
			bool ok = parse_param_obj(ctx, obj->error);
			if_goto(!ok, error);

		} else {
			ufa_debug("Bypassing: %s (size %d)", attr, tok->size);
			ctx->cursor++;
			ctx->cursor += tok->size;
		}
	}

	// FIXME method or result or error
//	if (!has_method) {
//		return UFA_JSONRPC_INVALID;
//	}

	return UFA_JSON_OK;
error:
	return UFA_JSONRPC_INVALID;
}

