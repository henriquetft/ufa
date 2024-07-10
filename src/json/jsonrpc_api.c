/* ========================================================================== */
/* Copyright (c) 2023-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* API for communicating with JSON-RPC server                                 */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "jsonrpc_api.h"
#include "core/repo.h"
#include "jsonrpc_parser.h"
#include "jsonrpc_server.h"
#include "util/error.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/string.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

/** Max chars in a JSON-RPC response */
#define MAX_RESPONSE_SIZE  4096


struct ufa_jsonrpc_api
{
	int socket_fd;
};


/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void request_socket(struct ufa_jsonrpc_api *obj,
			   const char *msg_to_send,
			   char *response_buf);

static bool request_jsonrpc(ufa_jsonrpc_api_t *api,
			    const char *msg,
			    struct ufa_jsonrpc **jsonrpc,
			    struct ufa_error **error);

/* ========================================================================== */
/* FUNCTIONS FROM jsonrpc.api.h                                               */
/* ========================================================================== */

ufa_jsonrpc_api_t *ufa_jsonrpc_api_init(struct ufa_error **error)
{
	ufa_return_val_iferror(error, NULL);

	struct ufa_jsonrpc_api *obj = NULL;

	struct sockaddr_un addr;
	int ret;
	int data_socket;

	data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (data_socket == -1) {
		perror("socket");
		ufa_error("%s: %s", __func__, strerror(errno));
		ufa_error_new(error, UFA_ERROR_INTERNAL, strerror(errno));
		return NULL;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	ret = connect(data_socket,
		      (const struct sockaddr *) &addr,
		      sizeof(struct sockaddr_un));
	if (ret == -1) {
		// FIXME put an error code here
		ufa_error_new(error, 0, "The JSRON-RPC server is down!");
		return NULL;
	}

	obj = ufa_malloc(sizeof *obj);
	obj->socket_fd = data_socket;
	return obj;
}


bool ufa_jsonrpc_api_settag(ufa_jsonrpc_api_t *api,
			    const char *filepath,
			    const char *tag,
			    struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool result = false;

	const char *str_json =
	    "{"
	    "    \"params\" : { \"filepath\" : \"%s\", \"tag\" : \"%s\" }, "
	    "    \"jsonrpc\": \"2.0\","
	    "    \"id\" : \"%s\","
	    "     \"method\": \"settag\""
	    "}";

	char *msg = ufa_str_sprintf(str_json, filepath, tag, "id-xpto-123");
	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	if (ufa_hashtable_size(rpc->error) == 0) {
		result = true;
	}

end:
	ufa_free(msg);
	ufa_jsonrpc_free(rpc);
	return result;
}

struct ufa_list *ufa_jsonrpc_api_listtags(ufa_jsonrpc_api_t *api,
					  const char *repodir,
					  struct ufa_error **error)
{
	ufa_return_val_iferror(error, NULL);

	struct ufa_list *result = NULL;

	const char *str_json = "{"
			       "    \"params\" : { \"repodir\" : \"%s\" } "
			       "    \"jsonrpc\": \"2.0\","
			       "    \"id\" : \"%s\","
			       "    \"method\": \"listtags\""
			       "}";

	char *msg = ufa_str_sprintf(str_json, repodir, "id-xpto-123");

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	if (ufa_hashtable_size(rpc->error) == 0) {
		struct ufa_list *p =
		    (struct ufa_list *) ufa_hashtable_get(rpc->result, "value");
		result = ufa_list_clone(p, (ufa_list_cpydata_fn_t) ufa_str_dup,
					ufa_free);
	}
end:
	ufa_free(msg);
	ufa_jsonrpc_free(rpc);
	return result;
}


bool ufa_jsonrpc_api_gettags(ufa_jsonrpc_api_t *api,
			     const char *filepath,
			     struct ufa_list **list,
			     struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool result = false;

	const char *str_json = "{"
			       "    \"params\" : { \"filepath\" : \"%s\" } "
			       "    \"jsonrpc\": \"2.0\","
			       "    \"id\" : \"%s\","
			       "    \"method\": \"gettags\""
			       "}";

	char *msg = ufa_str_sprintf(str_json, filepath, "id-xpto-123");

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	if (ufa_hashtable_size(rpc->error) > 0) {
		goto end;
	}

	struct ufa_list *p =
	    (struct ufa_list *) ufa_hashtable_get(rpc->result, "value");

	*list = ufa_list_clone(p,
			       (ufa_list_cpydata_fn_t) ufa_str_dup, ufa_free);
	result = true;
end:
	ufa_free(msg);
	ufa_jsonrpc_free(rpc);
	return result;
}


int ufa_jsonrpc_api_inserttag(ufa_jsonrpc_api_t *api,
			      const char *repodir,
			      const char *tag,
			      struct ufa_error **error)
{
	ufa_return_val_iferror(error, -1);

	long id_tag = -1;
	const char *str_json =
	    "{"
	    "    \"jsonrpc\": \"2.0\","
	    "    \"id\" : \"%s\","
	    "    \"method\": \"inserttag\","
	    "    \"params\" : { \"repodir\" : \"%s\", \"tag\" : \"%s\"  } "
	    "}";

	char *msg = ufa_str_sprintf(str_json, "id-xpto-123", repodir, tag);

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);


	if (ufa_hashtable_size(rpc->error) > 0) {
		goto end;
	}

	long *p = (long *) ufa_hashtable_get(rpc->result, "value");
	if (p == NULL) {
		goto end;
	}

	id_tag = *p;
end:
	ufa_free(msg);
	ufa_jsonrpc_free(rpc);
	return (int) id_tag;
}

bool ufa_jsonrpc_api_cleartags(ufa_jsonrpc_api_t *api,
			       const char *filepath,
			       struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool result = false;

	char *str_json = "{"
			 "    \"jsonrpc\": \"2.0\","
			 "    \"id\" : \"%s\","
			 "    \"method\": \"cleartags\","
			 "    \"params\" : { \"filepath\" : \"%s\" } "
			 "}";

	char *msg = ufa_str_sprintf(str_json, "id-xpto-123", filepath);

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	bool *p = (bool *) ufa_hashtable_get(rpc->result, "value");
	if (p) {
		result = *p;
	}

end:
	ufa_jsonrpc_free(rpc);
	ufa_free(msg);

	return result;

}

bool ufa_jsonrpc_api_unsettag(ufa_jsonrpc_api_t *api,
			      const char *filepath,
			      const char *tag,
			      struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool result = false;

	const char *str_json =
	    "{"
	    "    \"jsonrpc\": \"2.0\","
	    "    \"id\" : \"%s\","
	    "    \"method\": \"unsettag\","
	    "    \"params\" : { \"filepath\" : \"%s\", \"tag\" : \"%s\" } "
	    "}";

	char *msg = ufa_str_sprintf(str_json, "id-xpto-123", filepath, tag);

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	bool *p = (bool *) ufa_hashtable_get(rpc->result, "value");
	if (p) {
		result = *p;
	}
end:
	ufa_jsonrpc_free(rpc);
	ufa_free(msg);

	return result;
}


bool ufa_jsonrpc_api_setattr(ufa_jsonrpc_api_t *api,
			     const char *filepath,
			     const char *attribute,
			     const char *value,
			     struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool result = false;

	const char *str_json = "{"
			       "  \"jsonrpc\": \"2.0\","
			       "  \"id\" : \"%s\","
			       "  \"method\": \"setattr\","
			       "  \"params\" : { \"filepath\" : \"%s\", "
			       "                 \"attribute\" : \"%s\", "
			       "                 \"value\" : \"%s\" "
			       "               } "
			       "}";

	char *msg = ufa_str_sprintf(str_json,
				    "id-xpto-123",
				    filepath,
				    attribute,
				    value);
	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	bool *p = (bool *) ufa_hashtable_get(rpc->result, "value");
	if (p) {
		result = *p;
	}
end:
	ufa_jsonrpc_free(rpc);
	ufa_free(msg);
	return result;
}


struct ufa_list *ufa_jsonrpc_api_getattr(ufa_jsonrpc_api_t *api,
					 const char *filepath,
					 struct ufa_error **error)
{
	ufa_return_val_iferror(error, NULL);

	struct ufa_list *result = NULL;

	const char *str_json = "{"
			       "    \"params\" : { \"filepath\" : \"%s\" } "
			       "    \"jsonrpc\": \"2.0\","
			       "    \"id\" : \"%s\","
			       "    \"method\": \"getattr\""
			       "}";

	char *msg = ufa_str_sprintf(str_json, filepath, "id-xpto-123");

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	if (ufa_hashtable_size(rpc->error) == 0) {
		ufa_hashtable_t *attrs =
		    (ufa_hashtable_t *) ufa_hashtable_get(rpc->result, "value");

		struct ufa_list *attrs_keys = ufa_hashtable_keys(attrs);
		for (UFA_LIST_EACH(i, attrs_keys)) {
			char *key = (char *) i->data;
			char *value = (char *) ufa_hashtable_get(attrs, key);

			struct ufa_repo_attr *attr =
				ufa_calloc(1, sizeof *attr);
			attr->attribute = ufa_str_dup(key);
			attr->value = ufa_str_dup(value);
			result = ufa_list_append(result, attr);
			// FIXME isso gera memory leak

		}
		ufa_list_free(attrs_keys);
	}
end:
	ufa_free(msg);
	ufa_jsonrpc_free(rpc);
	return result;
}


bool ufa_jsonrpc_api_unsetattr(ufa_jsonrpc_api_t *api,
			       const char *filepath,
			       const char *attribute,
			       struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool result = false;

	const char *str_json = "{"
			       "    \"params\" : { \"filepath\" : \"%s\", "
			       "                   \"attribute\" : \"%s\"  } "
			       "    \"jsonrpc\": \"2.0\","
			       "    \"id\" : \"%s\","
			       "    \"method\": \"unsetattr\""
			       "}";

	char *msg = ufa_str_sprintf(str_json, filepath,
				    attribute, "id-xpto-123");

	struct ufa_jsonrpc *rpc = NULL;
	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	bool *p = (bool *) ufa_hashtable_get(rpc->result, "value");
	if (p) {
		result = *p;
	}
end:
	ufa_jsonrpc_free(rpc);
	ufa_free(msg);

	return result;
}


struct ufa_list *ufa_jsonrpc_api_search(ufa_jsonrpc_api_t *api,
					struct ufa_list *repo_dirs,
					struct ufa_list *filter_attr,
					struct ufa_list *tags,
					bool include_repo_from_config,
					struct ufa_error **error)
{
	ufa_return_val_iferror(error, NULL);

	struct ufa_list *result = NULL;

	// Dynamic allocated variables
	struct ufa_list *attr_str_list = NULL;
	struct ufa_jsonrpc *rpc        = NULL;
	char *filter_str               = NULL;
	char *tags_str                 = NULL;
	char *repo_dirs_str            = NULL;
	char *msg                      = NULL;

	const char *attr_format =
	    "{ \"attribute\": \"%s\", \"value\": \"%s\", \"matchmode\": %d }";
	const char *str_json =
	    "{"
	    " \"params\" : { \"repo_dirs\" : [ %s ],"
	    "                \"filter_attrs\" : [ %s ],"
	    "                \"tags\" : [ %s ],"
	    "                \"include_repo_from_config\" : %s }, "
	    "  \"jsonrpc\": \"2.0\","
	    "  \"id\" : \"%s\","
	    "   \"method\": \"search\""
	    "}";

	for (UFA_LIST_EACH(i, filter_attr)) {
		struct ufa_repo_filterattr *f =
		    (struct ufa_repo_filterattr *) i->data;
		char *new_str = ufa_str_sprintf(attr_format, f->attribute,
						f->value, f->matchmode);
		attr_str_list =
		    ufa_list_append2(attr_str_list, new_str, ufa_free);
	}

	filter_str = ufa_str_join_list(attr_str_list, ", ", NULL, NULL);
	tags_str = ufa_str_join_list(tags, ", ", "\"", "\"");
	repo_dirs_str = ufa_str_join_list(repo_dirs, ", ", "\"", "\"");
	msg = ufa_str_sprintf(str_json,
			      repo_dirs_str,
			      filter_str,
			      tags_str,
			      (include_repo_from_config) ? "true" : "false",
			      "id-xpto-123");


	bool ok = request_jsonrpc(api, msg, &rpc, error);
	if_goto(!ok, end);

	struct ufa_list *list_value =
	    (struct ufa_list *) ufa_hashtable_get(rpc->result, "value");

	if (list_value) {
		result = ufa_list_clone(list_value,
					(ufa_list_cpydata_fn_t) ufa_str_dup,
					ufa_free);
	}
end:
	ufa_free(tags_str);
	ufa_free(repo_dirs_str);
	ufa_free(filter_str);
	ufa_free(msg);
	ufa_list_free(attr_str_list);
	ufa_jsonrpc_free(rpc);

	return result;
}

void ufa_jsonrpc_api_close(ufa_jsonrpc_api_t *api, struct ufa_error **error)
{
	ufa_return_if(api == NULL);

	close(api->socket_fd);
	ufa_free(api);
	ufa_debug("Closing JSRON-RPC API");
}

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static void request_socket(struct ufa_jsonrpc_api *obj,
			   const char *msg_to_send,
			   char *response_buf)
{
	int ret;

	ufa_debug("Writting msg to socket: %s", msg_to_send);
	ret = write(obj->socket_fd, msg_to_send, strlen(msg_to_send) + 1);

	if (ret < 0) {
		// errno is set
	}

	ret = read(obj->socket_fd, response_buf, MAX_RESPONSE_SIZE);
	if (ret < 0) {
		// errno is set
	}

	ufa_debug("Received msg: %s", response_buf);
}



static bool request_jsonrpc(ufa_jsonrpc_api_t *api,
			    const char *msg,
			    struct ufa_jsonrpc **jsonrpc,
			    struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	char response[MAX_RESPONSE_SIZE] = "";
	request_socket(api, msg, response);

	enum ufa_parser_result r;
	if ((r = ufa_jsonrpc_parse(response, jsonrpc)) != UFA_JSON_OK) {
		ufa_error_new(error,
			      UFA_ERROR_INTERNAL,
			      "Error parsing JSONRPC response: %d", r);
		return false;
	}

	struct ufa_jsonrpc *rpc = *jsonrpc;

	if (rpc->error && ufa_hashtable_size(rpc->error) > 0) {
		ufa_debug("RPC reponse error");
		int code = UFA_ERROR_INTERNAL;
		char *message = "Error parsing JSONRPC response: %d";

		if (ufa_hashtable_has_key(rpc->error, "code") &&
		    ufa_hashtable_has_key(rpc->error, "message")) {
			code = *((int *) ufa_hashtable_get(rpc->error, "code"));
			message =
			    ((char *) ufa_hashtable_get(rpc->error, "message"));
		}

		ufa_error_new(error, code, message, code);
		if (*error) {
			ufa_error((*error)->message);
		}

		return false;
	}

	return true;
}
