/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of ufatag command line utility.                             */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/repo.h"
#include "tools/cli.h"
#include "util/list.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/string.h"
#include "json/jsonrpc_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

/* ========================================================================== */
/* FUNCTIONS -- DECLARATION                                                   */
/* ========================================================================== */

static void print_usage(FILE *stream);

static void print_usage_set(FILE *stream);
static void print_usage_unset(FILE *stream);
static void print_usage_list(FILE *stream);
static void print_usage_clear(FILE *stream);
static void print_usage_list_all(FILE *stream);
static void print_usage_create(FILE *stream);

static int handle_unset();
static int handle_set();
static int handle_list();
static int handle_clear();
static int handle_list_all();
static int handle_create();


/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static char *repository = NULL;
static ufa_jsonrpc_api_t *api = NULL;

char *commands[] = {
    "set", "unset", "list",
    "clear", "list-all", "create"
};

help_command_fn_t help_commands[] = {
    print_usage_set,   print_usage_unset,    print_usage_list,
    print_usage_clear, print_usage_list_all, print_usage_create,
};
handle_command_fn_t handle_commands[] = {
    handle_set,	  handle_unset,	   handle_list,
    handle_clear, handle_list_all, handle_create,
};

static int get_and_validate_repository()
{
	if (repository == NULL) {
		repository = ufa_util_get_current_dir();
		ufa_debug("Using CWD as repository: %s", repository);
	}

	int ret;
	if (!ufa_repo_isrepo(repository)) {
		fprintf(stderr, "error: %s is not a repository path\n",
			repository);
		ret = EXIT_FAILURE;
	} else {
		ret = EX_OK;
	}

	return ret;
}

/* ========================================================================== */
/* IMPLEMENTATION                                                             */
/* ========================================================================== */

static void print_usage(FILE *stream)
{
	fprintf(stream, "\nUsage: %s [OPTIONS] [COMMAND]\n", program_name);
	fprintf(stream, "\nCLI tool for managing tags of files\n");
	fprintf(stream,
		"\nOPTIONS\n"
		"  -h\t\tPrint this help and quit\n"
		"  -v\t\tPrint version information and quit\n"
		"  -r DIR\tRepository dir. Default is current dir\n"
		"  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
		"\n"
		"COMMANDS\n"
		"  set\t\tSet tags on file\n"
		"  unset\t\tUnset tags on file\n"
		"  list\t\tList the tags on file\n"
		"  clear\t\tUnset all tags on file\n"
		"  list-all\tList all tags\n"
		"  create\tCreate a tag\n"
		"\n"
		"Run '%s COMMAND -h' for more information on a command.\n"
		"\n",
		program_name);
}

static void print_usage_set(FILE *stream)
{
	printf("\nUsage:  %s set FILE TAG\n", program_name);
	printf("\nSet tags on file\n\n");
}

static void print_usage_unset(FILE *stream)
{
	printf("\nUsage:  %s unset FILE TAG\n", program_name);
	printf("\nUnset tags on file\n\n");
}

static void print_usage_list(FILE *stream)
{
	printf("\nUsage:  %s list FILE\n", program_name);
	printf("\nList the tags on file\n\n");
}

static void print_usage_clear(FILE *stream)
{
	printf("\nUsage:  %s clear FILE\n", program_name);
	printf("\nUnset all tags on file\n\n");
}

static void print_usage_list_all(FILE *stream)
{
	printf("\nUsage:  %s list-all\n", program_name);
	printf("\nList all tags of repository\n\n");
}

static void print_usage_create(FILE *stream)
{
	printf("\nUsage:  %s create TAG\n", program_name);
	printf("\nCreate a tag\n\n");
}

/* set tag on file */
static int handle_set()
{
	if (!HAS_MORE_ARGS(2)) {
		print_usage_set(stderr);
		return EX_USAGE;
	}

	char filepath[PATH_MAX];
	ufa_util_abspath2(NEXT_ARG, filepath);
	char *new_tag = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = ufa_jsonrpc_api_settag(api, filepath, new_tag, &error);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

/** remove tag from a file */
static int handle_unset()
{
	if (!HAS_MORE_ARGS(2)) {
		print_usage_unset(stderr);
		return EX_USAGE;
	}

	char filepath[PATH_MAX];
	ufa_util_abspath2(NEXT_ARG, filepath);
	char *tag = NEXT_ARG;


	struct ufa_error *error = NULL;
	bool is_ok = ufa_jsonrpc_api_unsettag(api, filepath, tag, &error);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

/* print all tags of a file */
static int handle_list()
{
	if (!HAS_NEXT_ARG) {
		print_usage_list(stderr);
		return EX_USAGE;
	}

	char filepath[PATH_MAX];
	ufa_util_abspath2(NEXT_ARG, filepath);

	/* FIXME must fail if file does not exist */
	struct ufa_list *list = NULL;
	struct ufa_error *error = NULL;
	bool is_ok = ufa_jsonrpc_api_gettags(api, filepath, &list, &error);

	int ret;

	if (is_ok) {
		for (UFA_LIST_EACH(iter, list)) {
			printf("%s\n", (char *) iter->data);
		}
		ufa_list_free(list);
		ret = EX_OK;
	} else {
		ret = EXIT_FAILURE;
		ufa_error_print_and_free(error);
	}
	return ret;
}

/**
 * Handle clear command.
 *
 * Removes all tags from a file.
 */
static int handle_clear()
{
	if (!HAS_NEXT_ARG) {
		print_usage_clear(stderr);
		return EX_USAGE;
	}

	char filepath[PATH_MAX];
	ufa_util_abspath2(NEXT_ARG, filepath);

	struct ufa_error *error = NULL;
	bool is_ok = ufa_jsonrpc_api_cleartags(api, filepath, &error);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

/**
 * Handle list-all command.
 * Prints all tags
 */
static int handle_list_all()
{
	int ret = get_and_validate_repository();
	if (ret != EX_OK) {
		return ret;
	}
	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_jsonrpc_api_listtags(api, repository, &error);
	bool is_ok = (error == NULL);
	if (is_ok) {
		for (UFA_LIST_EACH(iter, list)) {
			printf("%s\n", (char *) iter->data);
		}
		ufa_list_free(list);
		ret = EX_OK;
	} else {
		ret = EXIT_FAILURE;
		ufa_error_print_and_free(error);
	}
	return ret;
}

/**
 * Handle create command.
 */
static int handle_create()
{
	if (!HAS_NEXT_ARG) {
		print_usage_create(stderr);
		return EX_USAGE;
	}

	int ret = get_and_validate_repository();
	if (ret != EX_OK) {
		return ret;
	}

	char *tag = NEXT_ARG;
	struct ufa_error *error = NULL;
	bool is_ok = (ufa_jsonrpc_api_inserttag(api, repository, tag, &error) > 0);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}


int main(int argc, char *argv[])
{
	program_name = argv[0];
	global_args = argc;
	global_argv = argv;

	int opt;

	bool error_usage = false;
	int exit_status = EX_OK;
	int r = 0, log = 0;

	while ((opt = getopt(argc, argv, ":l:r:hv")) != -1 && !error_usage) {
		switch (opt) {
		case 'r':
			if (r) {
				error_usage = true;
			} else {
				r = 1;
				repository = ufa_str_dup(optarg);
			}
			break;
		case 'v':
			printf("%s\n", program_version);
			exit_status = EX_OK;
			goto end;
		case 'h':
			if (HAS_PREV_ARGS(1)) {
				exit_status = handle_help_command(
				    PREV_ARG(1), ARRAY_SIZE(help_commands));
			} else {
				print_usage(stdout);
				exit_status = EX_OK;
			}
			goto end;
		case 'l':
			if (log) {
				error_usage = true;
			} else {
				log = 1;
				enum ufa_log_level level =
				    ufa_log_level_from_str(optarg);
				ufa_log_setlevel(level);
				ufa_debug("LOG LEVEL: %s\n", optarg);
			}
			break;
		case '?':
			error_usage = true;
			fprintf(stderr, "unknown option: %c\n", optopt);
			break;

		default:
			error_usage = true;
		}
	}

	if (error_usage) {
		print_usage(stderr);
		exit_status = EX_USAGE;
		goto end;
	}

	if (!HAS_NEXT_ARG) {
		print_usage(stderr);
		exit_status = EX_USAGE;
		goto end;
	}

	// Start JSON-RPC API
	struct ufa_error *err_api = NULL;
	api = ufa_jsonrpc_api_init(&err_api);
	ufa_error_exit(err_api, EX_UNAVAILABLE);

	char *command = NEXT_ARG;
	exit_status = handle_command(command, ARRAY_SIZE(commands));

end:
	ufa_free(repository);
	ufa_jsonrpc_api_close(api, NULL);
	return exit_status;
}
