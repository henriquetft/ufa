/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of ufaattr command line utility.                            */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#include "tools/cli.h"
#include "core/repo.h"
#include "core/data.h"
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
static void print_usage_get(FILE *stream);
static void print_usage_list(FILE *stream);
static void print_usage_describe(FILE *stream);

static int handle_set();
static int handle_unset();
static int handle_get();
static int handle_list();
static int handle_describe();


/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

char *commands[] = {
    "set", "unset", "get",
    "list", "describe",
};

help_command_f help_commands[] = {
    print_usage_set,  print_usage_unset, print_usage_get,
    print_usage_list, print_usage_describe,
};
handle_command_f handle_commands[] = {
    handle_set, handle_unset, handle_get,
    handle_list, handle_describe,
};

/* ========================================================================== */
/* IMPLEMENTATION                                                             */
/* ========================================================================== */

static void print_usage(FILE *stream)
{
	fprintf(stream, "\nUsage: %s [OPTIONS] [COMMAND]\n", program_name);
	fprintf(stream, "\nCLI tool for managing attributes of files\n");
	fprintf(stream,
		"\nOPTIONS\n"
		"  -h\t\tPrint this help and quit\n"
		"  -v\t\tPrint version information and quit\n"
		"  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
		"\n"
		"COMMANDS\n"
		"  set\t\tSet attributes on file\n"
		"  unset\t\tUnset attributes on file\n"
		"  get\t\tGet the value of an attribute\n"
		"  list\t\tList attributes of a file\n"
		"  describe\tList attributes and values of a file\n"
		"\n"
		"Run '%s COMMAND -h' for more information on a command.\n"
		"\n",
		program_name);
}

static void print_usage_set(FILE *stream)
{
	printf("\nUsage:  %s set FILE ATTRIBUTE VALUE\n", program_name);
	printf("\nSet attributes on file\n\n");
}

static void print_usage_unset(FILE *stream)
{
	printf("\nUsage:  %s unset FILE ATTRIBUTE\n", program_name);
	printf("\nUnset attributes on file\n\n");
}

static void print_usage_get(FILE *stream)
{
	printf("\nUsage:  %s get FILE ATTRIBUTE\n", program_name);
	printf("\nGet the value of an attribute\n\n");
}

static void print_usage_list(FILE *stream)
{
	printf("\nUsage:  %s list FILE\n", program_name);
	printf("\nList attributes of a file\n\n");
}

static void print_usage_describe(FILE *stream)
{
	printf("\nUsage:  %s describe FILE\n", program_name);
	printf("\nList attributes and values of a file\n\n");
}

static int handle_set()
{
	if (!HAS_MORE_ARGS(3)) {
		print_usage_set(stderr);
		return EX_USAGE;
	}

	char *file = NEXT_ARG;
	char *attr = NEXT_ARG;
	char *value = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = ufa_data_setattr(file, attr, value, &error);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

static int handle_unset()
{
	if (!HAS_MORE_ARGS(2)) {
		print_usage_unset(stderr);
		return EX_USAGE;
	}

	char *file = NEXT_ARG;
	char *attr = NEXT_ARG;

	struct ufa_error *error = NULL;
	bool is_ok = ufa_data_unsetattr(file, attr, &error);
	ufa_error_print_and_free(error);
	return is_ok ? EX_OK : EXIT_FAILURE;
}

static int handle_get()
{
	if (!HAS_MORE_ARGS(2)) {
		print_usage_get(stderr);
		return EX_USAGE;
	}

	char *file = NEXT_ARG;
	char *attr = NEXT_ARG;

	bool found = false;

	struct ufa_error *error = NULL;
	struct ufa_list *list_attrs = ufa_data_getattr(file, &error);
	ufa_error_print_and_free(error);
	for (UFA_LIST_EACH(i, list_attrs)) {
		struct ufa_repo_attr *attr_i = (struct ufa_repo_attr *)i->data;
		if (ufa_str_equals(attr_i->attribute, attr)) {
			printf("%s\n", attr_i->value);
			found = true;
		}
	}
	ufa_list_free_full(list_attrs, ufa_repo_attrfree);
	return (!error && found) ? EX_OK : EXIT_FAILURE;
}

static int handle_list()
{
	if (!HAS_MORE_ARGS(1)) {
		print_usage_list(stderr);
		return EX_USAGE;
	}

	char *file = NEXT_ARG;

	struct ufa_error *error = NULL;
	struct ufa_list *list_attrs = ufa_data_getattr(file, &error);
	ufa_error_print_and_free(error);
	for (UFA_LIST_EACH(i, list_attrs)) {
		printf("%s\n", ((struct ufa_repo_attr *)i->data)->attribute);
	}
	ufa_list_free_full(list_attrs, ufa_repo_attrfree);
	return !error ? EX_OK : EXIT_FAILURE;
}

static int handle_describe()
{
	if (!HAS_MORE_ARGS(1)) {
		print_usage_describe(stderr);
		return EX_USAGE;
	}

	char *file = NEXT_ARG;

	struct ufa_error *error = NULL;
	struct ufa_list *list_attrs = ufa_data_getattr(file, &error);
	ufa_error_print_and_free(error);
	for (UFA_LIST_EACH(i, list_attrs)) {
		struct ufa_repo_attr *attr_i = (struct ufa_repo_attr *)i->data;
		printf("%s\t%s\n", (char *) attr_i->attribute,
		       (char *) attr_i->value);
	}
	ufa_list_free_full(list_attrs, ufa_repo_attrfree);
	return !error ? EX_OK : EXIT_FAILURE;
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

	while ((opt = getopt(argc, argv, ":l:hv")) != -1 && !error_usage) {
		switch (opt) {
		case 'v':
			printf("%s\n", program_version);
			exit_status = EX_OK;
			goto end;
		case 'h':
			if (HAS_NEXT_ARG) {
				exit_status = handle_help_command(NEXT_ARG, ARRAY_SIZE(help_commands));
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

	exit_status = handle_command(command, ARRAY_SIZE(commands));
end:
	ufa_data_close();
	return exit_status;
}