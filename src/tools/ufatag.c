/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of ufatag command line utility.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include "core/repo.h"
#include "util/list.h"
#include "util/logging.h"
#include "util/misc.h"
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

static int handle_command(char *command);

static int handle_help_option(char *command);

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define HAS_NEXT_ARG (optind < global_args)
#define HAS_MORE_ARGS(num) (optind + num - 1 < global_args)
#define NEXT_ARG global_argv[optind++]

#define EXIT_COMMAND_NOT_FOUND 127

static int global_args = -1;
static char **global_argv = NULL;

static char *program_name = "";
static char *program_version = "0.1";

typedef int (*handle_command_f)();
typedef void (*help_command_f)(FILE *data);

static const int NUM_COMMANDS = 6;
static char *commands[] = {"set",   "unset",	"list",
			   "clear", "list-all", "create"};

static help_command_f help_commands[] = {
    print_usage_set,   print_usage_unset,    print_usage_list,
    print_usage_clear, print_usage_list_all, print_usage_create,
};
static handle_command_f handle_commands[] = {
    handle_set,	  handle_unset,	   handle_list,
    handle_clear, handle_list_all, handle_create,
};

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
		"\n"
		"COMMANDS\n"
		"  set\t\tSet tags on file\n"
		"  unset\t\tUnset tags on file\n"
		"  list\t\tList the tags on file\n"
		"  clear\t\tUnset all tags on file\n"
		"  list-all\tList all tags\n"
		"  create\tCreate a tag"
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

	char *file = NEXT_ARG;
	char *new_tag = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = ufa_repo_settag(file, new_tag, &error);
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

	char *file = NEXT_ARG;
	char *tag = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = ufa_repo_unsettag(file, tag, &error);
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

	char *arg = NEXT_ARG;

	/* FIXME must fail if file does not exist */
	struct ufa_list *list = NULL;
	struct ufa_error *error = NULL;
	bool is_ok = ufa_repo_gettags(arg, &list, &error);
	int ret;

	if (is_ok) {
		for (UFA_LIST_EACH(iter, list)) {
			printf("%s\n", (char *)iter->data);
		}
		ufa_list_free_full(list, free);
		ret = EX_OK;
	} else {
		ret = EXIT_FAILURE;
		ufa_error_print_and_free(error);
	}
	return ret;
}

/**
 * Handle clear command.
 * Removes all tags from a file.
 */
static int handle_clear()
{
	if (!HAS_NEXT_ARG) {
		print_usage_clear(stderr);
		return EX_USAGE;
	}

	char *file = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = ufa_repo_cleartags(file, &error);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

/**
 * Handle list-all command.
 * Prints all tags
 */
static int handle_list_all()
{
	int ret;
	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_listtags(&error);
	bool is_ok = (error == NULL);
	if (is_ok) {
		for (UFA_LIST_EACH(iter, list)) {
			printf("%s\n", (char *)iter->data);
		}
		ufa_list_free_full(list, free);
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

	char *tag = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = (ufa_repo_inserttag(tag, &error) > 0);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

static int handle_command(char *command)
{
	int exit_status = EXIT_COMMAND_NOT_FOUND;
	for (int pos = 0; pos < NUM_COMMANDS; pos++) {
		if (ufa_util_strequals(command, commands[pos])) {
			exit_status = handle_commands[pos]();
		}
	}
	if (exit_status == EXIT_COMMAND_NOT_FOUND) {
		fprintf(stderr, "\ninvalid command");
		fprintf(stderr, "\nSee %s -h\n", program_name);
	}
	return exit_status;
}

static int handle_help_option(char *command)
{
	int exit_code = EXIT_COMMAND_NOT_FOUND;
	for (int pos = 0; pos < NUM_COMMANDS; pos++) {
		if (ufa_util_strequals(command, commands[pos])) {
			help_commands[pos](stdout);
			exit_code = EX_OK;
		}
	}
	if (exit_code != EX_OK) {
		fprintf(stderr, "invalid command\n");
	}
	return exit_code;
}

int main(int argc, char *argv[])
{
	program_name = argv[0];
	global_args = argc;
	global_argv = argv;

	int opt;
	char *repository = NULL;

	bool error_usage = false;
	int exit_status = EX_OK;
	int r = 0, log = 0;

	while ((opt = getopt(argc, argv, ":r:hv")) != -1 && !error_usage) {
		switch (opt) {
		case 'r':
			if (r) {
				error_usage = true;
			} else {
				r = 1;
				repository = ufa_strdup(optarg);
			}
			break;
		case 'v':
			printf("%s\n", program_version);
			exit_status = EX_OK;
			goto end;
		case 'h':
			if (HAS_NEXT_ARG) {
				exit_status = handle_help_option(NEXT_ARG);
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

	char *command = NEXT_ARG;

	if (repository == NULL) {
		// repository = ufa_util_get_current_dir();
		// ufa_debug("Using CWD as repository: %s", repository);
		fprintf(stderr, "error: you must specify a repository path\n");
		exit_status = EXIT_FAILURE;
		goto end;
	}

	struct ufa_error *err = NULL;
	if (!ufa_repo_init(repository, &err)) {
		ufa_error_print_and_free(err);
		exit_status = 1;
		goto end;
	}
	exit_status = handle_command(command);

end:
	free(repository);
	return exit_status;
}
