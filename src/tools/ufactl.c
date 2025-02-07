/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of ufactl command line utility.                             */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/data.h"
#include "tools/cli.h"
#include "core/config.h"
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
static void print_usage_add(FILE *stream);
static void print_usage_remove(FILE *stream);
static void print_usage_list(FILE *stream);
static void print_usage_init(FILE *stream);

static int handle_add();
static int handle_remove();
static int handle_list();
static int handle_init();

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */


char *commands[] = {
    "add",
    "remove",
    "list",
    "init",
};

help_command_fn_t help_commands[] = {
    print_usage_add,
    print_usage_remove,
    print_usage_list,
    print_usage_init,
};

handle_command_fn_t handle_commands[] = {
    handle_add,
    handle_remove,
    handle_list,
    handle_init,
};


/* ========================================================================== */
/* IMPLEMENTATION                                                             */
/* ========================================================================== */

static void print_usage(FILE *stream)
{
	fprintf(stream, "\nUsage: %s [OPTIONS] [COMMAND]\n", program_name);
	fprintf(stream, "\nCLI tool to control UFA\n");
	fprintf(stream,
		"\nOPTIONS\n"
		"  -h\t\tPrint this help and quit\n"
		"  -v\t\tPrint version information and quit\n"
		"  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
		"\n"
		"COMMANDS\n"
		"  add\t\tAdd repository directory to watching list\n"
		"  remove\tRemove repository directory from watching list\n"
		"  list\t\tList current watched repositories\n"
		"  init\t\tInitialize repository\n"
		"\n"
		"Run '%s COMMAND -h' for more information on a command.\n"
		"\n",
		program_name);
}

static void print_usage_add(FILE *stream)
{
	fprintf(stream, "\nUsage:  %s add REPOSITORY \n", program_name);
	fprintf(stream, "\nAdd repository directory to watching list\n\n");
}

static void print_usage_remove(FILE *stream)
{
	fprintf(stream, "\nUsage:  %s remove REPOSITORY\n", program_name);
	fprintf(stream, "\nRemove repository directory from watching list\n\n");
}

static void print_usage_list(FILE *stream)
{
	fprintf(stream, "\nUsage:  %s list\n", program_name);
	fprintf(stream, "\nList current watched repositories\n\n");
}

static void print_usage_init(FILE *stream)
{
	fprintf(stream, "\nUsage:  %s init REPOSITORY_PATH\n", program_name);
	fprintf(stream, "\nCreate repository metadata when it does not exist"
			" \n\n");
}



static int handle_add()
{
	if (!HAS_MORE_ARGS(1)) {
		print_usage_add(stderr);
		return EX_USAGE;
	}

	char *dir = NEXT_ARG;
	struct ufa_error *error = NULL;
	bool is_ok = ufa_config_add_dir(dir, &error);
	ufa_error_print_and_free(error);

	if (is_ok) {
		char *abspath = ufa_util_abspath(dir);
		printf("Added %s\n", abspath);
		ufa_free(abspath);
		return EX_OK;
	}
	return EXIT_FAILURE;
}

static int handle_remove()
{
	int ret;

	if (!HAS_MORE_ARGS(1)) {
		print_usage_remove(stderr);
		return EX_USAGE;
	}
	char *dir = NEXT_ARG;
	char *dirpath = ufa_util_resolvepath(dir);

	struct ufa_error *error = NULL;
	bool is_ok = ufa_config_remove_dir(dir, &error);
	ufa_error_print_and_free(error);

	if (is_ok) {
		ret = EX_OK;
		printf("Removed %s\n", dirpath);
		return EX_OK;
	} else {
		ret = EXIT_FAILURE;
	}
	ufa_free(dirpath);
	return ret;
}

static int handle_list()
{
	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_config_dirs(false, &error);
	for (UFA_LIST_EACH(i, list)) {
		printf("%s\n", (char *) i->data);
	}

	ufa_error_print_and_free(error);
	ufa_list_free(list);
	return error ? EXIT_FAILURE : EX_OK;
}

static int handle_init()
{
	if (!HAS_MORE_ARGS(1)) {
		print_usage_init(stderr);
		return EX_USAGE;
	}
	struct ufa_error *error = NULL;
	char *dir = NEXT_ARG;
	ufa_data_init_repo(dir, &error);
	ufa_error_print_and_free(error);

	return error ? EXIT_FAILURE : EX_OK;
}


int main(int argc, char *argv[])
{
	program_name = argv[0];
	global_args  = argc;
	global_argv  = argv;

	bool error_usage = false;
	int exit_status  = EX_OK;
	int log          = 0;
	int opt;

	while ((opt = getopt(argc, argv, ":l:hv")) != -1
		&& !error_usage) {
		switch (opt) {
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
		default:
			error_usage = true;
		}
	}

	if (error_usage) {
		print_usage(stderr);
		exit_status = EX_USAGE;
	} else if (HAS_MORE_ARGS(1)) {
		exit_status = handle_command(NEXT_ARG, ARRAY_SIZE(commands));
	} else {
		print_usage(stderr);
		exit_status = EX_USAGE;
	}

end:
	return exit_status;
}
