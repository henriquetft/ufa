/* ========================================================================== */
/* Copyright (c) 2023-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of UFA JSON-RPC server.                                     */
/* Listens for JSON-RPC requests and calls functions according to the method  */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "jsonrpc_parser.h"
#include "jsonrpc_server.h"
#include "util/misc.h"
#include "util/string.h"
#include "core/data.h"
#include "core/repo.h"
#include "util/logging.h"
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

/** MAX chars for each read operation from socket */
#define CHUNK_SIZE 1024

static bool jsonrpc_server(int *fd);

struct ufa_jsonrpc_server {
	int socket_fd;
};

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static bool jsonrpc_server(int *fd);
static void *handle_connection(void *thread_data);
static void process_request(int fd, struct ufa_jsonrpc *rpc);
static void *get_param(struct ufa_jsonrpc *rpc, const char *param,
		       struct ufa_error **error);

static void handle_listtags(int fd, struct ufa_jsonrpc *rpc);
static void handle_settag(int fd, struct ufa_jsonrpc *rpc);
static void handle_cleartags(int fd, struct ufa_jsonrpc *rpc);
static void handle_gettags(int fd, struct ufa_jsonrpc *rpc);
static void handle_inserttag(int fd, struct ufa_jsonrpc *rpc);
static void handle_unsettag(int fd, struct ufa_jsonrpc *rpc);
static void handle_setattr(int fd, struct ufa_jsonrpc *rpc);
static void handle_unsetattr(int fd, struct ufa_jsonrpc *rpc);
static void handle_getattr(int fd, struct ufa_jsonrpc *rpc);
static void handle_search(int fd, struct ufa_jsonrpc *rpc);

static void send_response_list_str(int fd, const char *id,
				   struct ufa_list *elements);
static void send_response_bool(int fd, const char *id, bool value);
static void send_response_int(int fd, const char *id, int value);
static void send_response_objs_attr(int fd, const char *id,
				    struct ufa_list *elements);
static void send_error_response(int fd, const char *id, int code,
				const char *message);


/* ========================================================================== */
/* FUNCTIONS FROM jsonrpc_parser.h                                            */
/* ========================================================================== */

ufa_jsonrpc_server_t *ufa_jsonrpc_server_new()
{
	struct ufa_jsonrpc_server *obj = NULL;
	obj = ufa_malloc(sizeof *obj);
	obj->socket_fd = -1;
	return obj;
}

void ufa_jsonrpc_server_start(ufa_jsonrpc_server_t *server,
			      struct ufa_error **error)
{
	ufa_return_iferror(error);

	if (server) {
		jsonrpc_server(&server->socket_fd);
	}
}

void ufa_jsonrpc_server_stop(ufa_jsonrpc_server_t *server,
			     struct ufa_error **error)
{
	ufa_return_iferror(error);

	shutdown(server->socket_fd, SHUT_RDWR);
	close(server->socket_fd);
	// FIXME check errors
}

void ufa_jsonrpc_server_free(ufa_jsonrpc_server_t *server)
{
	ufa_free(server);
}

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static bool jsonrpc_server(int *fd)
{
	struct sockaddr_un addr;
	int ret = -1;
	int listen_socket;

	unlink(SOCKET_PATH);

	listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	*fd = listen_socket;

	ufa_debug("JSONRPC Server Listen Socket: %d", listen_socket);
	if (listen_socket == -1) {
		perror("socket");
		return false;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(listen_socket, (const struct sockaddr *) &addr,
		   sizeof(struct sockaddr_un));
	if (ret == -1) {
		perror("bind");
		return false;
	}

	ret = listen(listen_socket, 20);
	if (ret == -1) {
		perror("listen"); // FIXME
		return false;
	}

	ufa_debug("JSONRPC Server Waiting for connections...");
	int cfd;
	while ((cfd = accept(listen_socket, NULL, NULL)) != -1) {
		ufa_debug("New connection: %d", cfd);
		pthread_t thread;
		int ret = pthread_create(&thread,
					 NULL,
					 handle_connection,
					 &cfd);
		if (ret != 0) {
			// FIXME
			fprintf(stderr, "Error creating thread\n");
		}
	}

	close(listen_socket);

	unlink(SOCKET_PATH);

	return true;
}

static void *handle_connection(void *thread_data)
{
	char *buf = NULL;
	int last_size = 0;

	char part[CHUNK_SIZE] = "";
	int fd = *((int *) thread_data);
	int ret;

	ufa_debug("Start reading socket: %d\n", fd);

	while ((ret = read(fd, part, CHUNK_SIZE)) > 0) {
		part[ret] = '\0';
		if (strlen(part) == 0) {
			continue;
		}
		ufa_debug("Received %d chars: <%s>", ret, part);

		int old_size = last_size;
		last_size += strlen(part) + 1;
		buf = ufa_realloc(buf, last_size);
		memset(buf + old_size, 0,
		       sizeof(char) * (last_size - old_size));
		strcat(buf, part); // FIXME !!!

		ufa_debug("Passing arg to parser: <%s>\n", buf);
		struct ufa_jsonrpc *rpc = NULL;
		enum ufa_parser_result p = ufa_jsonrpc_parse(buf, &rpc);

		if (p == UFA_JSON_OK) {
			ufa_debug("Received the entire request: '%s'", buf);
			ufa_debug("RPC Method: '%s'", rpc->method);
			process_request(fd, rpc);
			ufa_free(buf);
			last_size = 0;
			buf = NULL;

		} else if (p == UFA_JSON_PART) {
			ufa_debug("Received part of request: '%s'", buf);
		} else {
			ufa_error("Error reading");
			ufa_free(buf);
			last_size = 0;
			buf = NULL;
		}

		ufa_jsonrpc_free(rpc);
		rpc = NULL;
		ufa_debug("Awaiting next data from socket: %d  ...", fd);
	}

	ufa_debug("End of reading loop: %d\n", ret);

	int r = close(fd);
	if (r != 0) {
		perror("close"); // FIXME
	}

	ufa_debug("Exiting thread for sock: %d", fd);

	return NULL;
}

static void process_request(int fd, struct ufa_jsonrpc *rpc)
{
	if (ufa_str_equals(rpc->method, "listtags")) {
		handle_listtags(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "gettags")) {
		handle_gettags(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "settag")) {
		handle_settag(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "cleartags")) {
		handle_cleartags(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "inserttag")) {
		handle_inserttag(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "unsettag")) {
		handle_unsettag(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "setattr")) {
		handle_setattr(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "unsetattr")) {
		handle_unsetattr(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "getattr")) {
		handle_getattr(fd, rpc);

	} else if (ufa_str_equals(rpc->method, "search")) {
		handle_search(fd, rpc);
	}
}

static void *get_param(struct ufa_jsonrpc *rpc, const char *param,
		       struct ufa_error **error)
{
	ufa_hashtable_t *table = rpc->params;
	void *value = ufa_hashtable_get(table, param);

	ufa_debug("Has key '%s'\n", param, ufa_hashtable_has_key(table, param));
	if (!ufa_hashtable_has_key(table, param)) {
		ufa_error_new(error, JSONRPC_INVALID_PARAMS,
			      "Missing parameter '%s'", param);
		return NULL;
	} else {
		return value;
	}
}

static void handle_listtags(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags = NULL;

	char *repodir = (char *) get_param(rpc, "repodir", &error);
	if_goto(error != NULL, error);

	tags = ufa_data_listtags(repodir, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}

	send_response_list_str(fd, rpc->id, tags);
	ufa_list_free(tags);
	return;
error:
	ufa_list_free(tags);
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_gettags(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	struct ufa_list *list = NULL;

	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	ufa_data_gettags(filepath, &list, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}

	send_response_list_str(fd, rpc->id, list);
	ufa_list_free(list);
	return;
error:
	ufa_list_free(list);
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_settag(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	char *tag = (char *) get_param(rpc, "tag", &error);
	if_goto(error != NULL, error);

	bool ret = ufa_data_settag(filepath, tag, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}
	send_response_bool(fd, rpc->id, ret);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_cleartags(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;

	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	bool ret = ufa_data_cleartags(filepath, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}

	send_response_bool(fd, rpc->id, ret);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_inserttag(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	char *repodir = (char *) get_param(rpc, "repodir", &error);
	if_goto(error != NULL, error);

	char *tag = (char *) get_param(rpc, "tag", &error);
	if_goto(error != NULL, error);

	int id = ufa_data_inserttag(repodir, tag, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}
	send_response_int(fd, rpc->id, id);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_unsettag(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	char *tag = (char *) get_param(rpc, "tag", &error);
	if_goto(error != NULL, error);

	bool ret = ufa_data_unsettag(filepath, tag, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}
	send_response_bool(fd, rpc->id, ret);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_setattr(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	char *attribute = (char *) get_param(rpc, "attribute", &error);
	if_goto(error != NULL, error);

	char *value = (char *) get_param(rpc, "value", &error);
	if_goto(error != NULL, error);

	bool ret = ufa_data_setattr(filepath, attribute, value, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}
	send_response_bool(fd, rpc->id, ret);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_unsetattr(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	char *attribute = (char *) get_param(rpc, "attribute", &error);
	if_goto(error != NULL, error);

	bool ret = ufa_data_unsetattr(filepath, attribute, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}
	send_response_bool(fd, rpc->id, ret);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_getattr(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_error *error = NULL;
	struct ufa_list *attributes = NULL;

	char *filepath = (char *) get_param(rpc, "filepath", &error);
	if_goto(error != NULL, error);

	attributes = ufa_data_getattr(filepath, &error);
	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto error;
	}

	send_response_objs_attr(fd, rpc->id, attributes);

	ufa_list_free(attributes);
	return;
error:
	send_error_response(fd, rpc->id, error->code, error->message);
	ufa_error_free(error);
}

static void handle_search(int fd, struct ufa_jsonrpc *rpc)
{
	struct ufa_list *result = NULL;
	struct ufa_error *error = NULL;
	struct ufa_list *attributes = NULL;

	struct ufa_list *filter_attrs =
	    (struct ufa_list *) get_param(rpc, "filter_attrs", &error);
	if_goto(error != NULL, end);

	struct ufa_list *tags =
	    (struct ufa_list *) get_param(rpc, "tags", &error);
	if_goto(error != NULL, end);

	struct ufa_list *repo_dirs =
	    (struct ufa_list *) get_param(rpc, "repo_dirs", &error);
	if_goto(error != NULL, end);

	// FIXME como lida com a obrigatoriedade desse parametro ?
	bool *include_repo_from_cofig =
	    (bool *) get_param(rpc, "include_repo_from_config", &error);
	if_goto(error != NULL, end);

	for (UFA_LIST_EACH(i, filter_attrs)) {
		ufa_hashtable_t *table = (ufa_hashtable_t *) i->data;
		char *attr = ufa_hashtable_get(table, "attribute");
		char *val = ufa_hashtable_get(table, "value");
		enum ufa_repo_matchmode matchmode =
		    *((int *) ufa_hashtable_get(table, "matchmode"));
		struct ufa_repo_filterattr *fa =
		    ufa_repo_filterattr_new(attr, val, matchmode);
		attributes = ufa_list_append2(
		    attributes, fa,
		    (ufa_list_free_fn_t) ufa_repo_filterattr_free);
	}

	result = ufa_data_search(repo_dirs,
				 attributes,
				 tags,
				 *include_repo_from_cofig,
				 &error);

	if (error) {
		error->code = JSONRPC_INTERNAL_ERROR;
		goto end;
	}


end:
	if (error) {
		send_error_response(fd, rpc->id, error->code, error->message);
		ufa_error_free(error);
	} else {
		send_response_list_str(fd, rpc->id, result);
	}

	ufa_list_free(attributes);
	ufa_list_free(result);
}

static void send_error_response(int fd, const char *id, int code,
				const char *message)
{
	const char *response =
	    "{ \"jsonrpc\" : \"2.0\", "
	    "\"id\" : \"%s\","
	    " \"error\" : { \"code\": %d, \"message\": \"%s\" } }";
	char *buf = ufa_str_sprintf(response, STR_NOTNULL(id), code,
				    STR_NOTNULL(message));

	write(fd, buf, strlen(buf) + 1);
	ufa_free(buf);
}

static void send_response_list_str(int fd, const char *id,
				   struct ufa_list *elements)
{
	const char *response = "{ \"jsonrpc\" : \"2.0\", \"id\" : \"%s\", "
			       "\"result\" : { \"value\" : [ %s ] } }";
	char *str_list = ufa_str_join_list(elements, ", ", "\"", "\"");

	const char *i = STR_NOTNULL(id);
	char *buf = ufa_str_sprintf(response, i, str_list);

	write(fd, buf, strlen(buf) + 1);

	ufa_debug("Sending response: %s", buf);
	ufa_free(str_list);
	ufa_free(buf);
}

static void send_response_objs_attr(int fd, const char *id,
				    struct ufa_list *elements)
{
	const char *response = "{ \"jsonrpc\" : \"2.0\", \"id\" : \"%s\", "
			       "\"result\" : { \"value\" : { %s } } }";

	struct ufa_list *list = NULL;

	for (UFA_LIST_EACH(i, elements)) {
		struct ufa_repo_attr *e = (struct ufa_repo_attr *) i->data;
		char *str =
		    ufa_str_sprintf("\"%s\" : \"%s\"", e->attribute, e->value);
		list = ufa_list_append(list, str);
	}

	char *str_list = ufa_str_join_list(list, ", ", NULL, NULL);

	const char *i = STR_NOTNULL(id);
	char *buf = ufa_str_sprintf(response, i, str_list);

	write(fd, buf, strlen(buf) + 1);

	ufa_list_free_full(list, ufa_free);
	ufa_free(str_list);
	ufa_free(buf);
}

static void send_response_bool(int fd, const char *id, bool value)
{
	const char *response = "{ \"jsonrpc\" : \"2.0\", \"id\" : \"%s\", "
			       "\"result\" : { \"value\" : %s } }";

	const char *i = STR_NOTNULL(id);
	char *buf =
	    ufa_str_sprintf(response, i, (value == true) ? "true" : "false");

	write(fd, buf, strlen(buf) + 1);

	ufa_free(buf);
}

static void send_response_int(int fd, const char *id, int value)
{
	const char *response = "{ \"jsonrpc\" : \"2.0\", \"id\" : \"%s\", "
			       "\"result\" : { \"value\" : %d } }";

	const char *i = STR_NOTNULL(id);
	char *buf = ufa_str_sprintf(response, i, value);

	write(fd, buf, strlen(buf) + 1);

	ufa_free(buf);
}

