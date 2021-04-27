/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of ufatag command line utility.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logging.h"
#include "misc.h"
#include "repo.h"

// ufatag -r /home/henrique/teste set FILE NEWTAG
// ufatag clear FILE
// ufatag unset FILE TAG
// ufatag list FILE

#define HAS_MORE_ARGS (optind < global_args)
#define NEXT_ARG global_argv[optind++];

static char *program_name = "";
static char *program_version = "0.1";

static int global_args = -1;
static char **global_argv = NULL;


static void
print_usage(FILE *stream)
{
    fprintf(stream, "\nUsage: %s [OPTIONS] [COMMAND]\n", program_name);
    fprintf(stream,
        "\nOPTIONS\n"
        "  -h\t\tprint this help.\n"
        "  -v\t\tprint version.\n"
        "  -r DIR\tRepository dir. Default is current dir\n"
        "\n");
}

/** remove tag from a file */
static int
handle_unset()
{
    return 0;
}


/* print all tags of a file */
static int
handle_list()
{
    if (!HAS_MORE_ARGS) {
        printf("print usage\n");
        return -1;
    }

    char *arg = NEXT_ARG;

    if (ufa_util_strequals(arg, "help")) {
        printf("print help list\n");
    } else {
        ufa_list_t *list = ufa_repo_get_tags_for_file(arg);
        for (ufa_list_t *iter = list; iter != NULL; iter = iter->next) {
            printf("%s\n", (char *)iter->data);
        }
        ufa_list_free_full(list, free);
    }
	return 0;
}

/* set tag on file */
static int
handle_set()
{
    return 0;
}

/* remove all tags from a file */
static int
handle_clear()
{
    return 0;
}


static int
handle_command(char *command)
{
    int r = 0;
    if (ufa_util_strequals(command, "set")) {
        r = handle_set();
    } else if (ufa_util_strequals(command, "unset")) {
        r = handle_unset();
    } else if (ufa_util_strequals(command, "list")) {
        r = handle_list();
    } else if (ufa_util_strequals(command, "clear")) {
        r = handle_clear();
    } else {
        fprintf(stderr, "invalid command\n");
        r = 1;
    }
    return r;
}


int
main(int argc, char *argv[])
{
	program_name = argv[0];
    global_args = argc;
    global_argv = argv;

    int opt;
	char *repository = NULL;

	int r = 0, v = 0, h = 0, unknown = 0;
    while ((opt = getopt(argc, argv, ":r:hv")) != -1) {
        switch (opt) {
        case 'r':
			r = 1;
			repository = optarg;
            break;
        case 'v':
			v = 1;
            printf("%s\n", program_version);
            break;
        case 'h':
			h = 1;
            print_usage(stdout);
            break;
        case '?':
			unknown = 1;
            fprintf(stderr, "unknown option: %c\n", optopt);
            break;
        default:
            fprintf(stderr, "error\n");
        }
    }

	if (v || h || unknown) {
		return 0;
	} 

    if (!HAS_MORE_ARGS) {
        fprintf(stderr, "missing command\n");
        return 1;
    }

    char *command = NEXT_ARG;

	ufa_repo_init(repository);
    int ret = handle_command(command);
    return ret;
}
