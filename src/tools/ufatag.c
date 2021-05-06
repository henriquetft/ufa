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
#include <sysexits.h>
#include "util/logging.h"
#include "util/misc.h"
#include "core/repo.h"
#include "util/list.h"


/* ============================================================================================== */
/* FUNCTIONS                                                                                      */
/* ============================================================================================== */

static void
print_usage(FILE *stream);

static void
print_usage(FILE *stream);

static void
print_usage_unset(FILE *stream);

static void
print_usage_list(FILE *stream);

static void
print_usage_set(FILE *stream);

static void
print_usage_clear(FILE *stream);

static int
handle_unset();

static int
handle_set();

static int
handle_list();

static int
handle_clear();

static int
handle_command(char *command);

static int
handle_help_option(char *command);


/* ============================================================================================== */
/* VARIABLES AND DEFINITIONS                                                                      */
/* ============================================================================================== */

#define HAS_NEXT_ARG (optind < global_args)
#define HAS_MORE_ARGS(num) (optind+num-1 < global_args)
#define NEXT_ARG global_argv[optind++]

#define EXIT_COMMAND_NOT_FOUND 127

static int global_args = -1;
static char **global_argv = NULL;

static char *program_name = "";
static char *program_version = "0.1";

typedef int (*handle_command_f)();
typedef void (*help_command_f)(FILE *data);

static const int NUM_COMMANDS = 4;
static char *commands[] = { "set", "unset", "list", "clear" };

static help_command_f help_commands[] = { print_usage_set,
                                          print_usage_unset,
                                          print_usage_list,
                                          print_usage_clear
                                        };
static handle_command_f handle_commands[] = { handle_set,
                                              handle_unset,
                                              handle_list,
                                              handle_clear
                                            };


/* ============================================================================================== */
/* IMPLEMENTATION                                                                                 */
/* ============================================================================================== */

static void
print_usage(FILE *stream)
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
        "\n"
        "Run '%s COMMAND -h' for more information on a command.\n"
        "\n", program_name);
}

static void
print_usage_unset(FILE *stream)
{
    printf("\nUsage:  %s unset FILE TAG\n", program_name);
    printf("\nUnset tags on file\n\n");
}

static void
print_usage_list(FILE *stream)
{
    printf("\nUsage:  %s list FILE\n", program_name);
    printf("\nList the tags on file\n\n");
}

static void
print_usage_set(FILE *stream)
{
    printf("\nUsage:  %s set FILE TAG\n", program_name);
    printf("\nSet tags on file\n\n");
}

static void
print_usage_clear(FILE *stream)
{
    printf("\nUsage:  %s clear FILE\n", program_name);
    printf("\nUnset all tags on file\n\n");
}

/** remove tag from a file */
static int
handle_unset()
{
    if (!HAS_MORE_ARGS(2)) {
        print_usage_unset(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;
    char *tag = NEXT_ARG;

    bool ret = ufa_repo_unset_tag_on_file(file, tag);
    return ret ? EX_OK : EXIT_FAILURE;
}


/* print all tags of a file */
static int
handle_list()
{
    if (!HAS_NEXT_ARG) {
        print_usage_list(stderr);
        return EX_USAGE;
    }

    char *arg = NEXT_ARG;

    /* FIXME must fail if file does not exist */
    ufa_list_t *list = NULL;
    int ret = ufa_repo_get_tags_for_file(arg, &list);

    for (UFA_LIST_EACH(iter, list)) {
        printf("%s\n", (char *)iter->data);
    }
    ufa_list_free_full(list, free);

    return ret >= 0 ? EX_OK : EXIT_FAILURE;
}


/* set tag on file */
static int
handle_set()
{
    if (!HAS_MORE_ARGS(2)) {
        print_usage_set(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;
    char *new_tag = NEXT_ARG;

    bool ret = ufa_repo_set_tag_on_file(file, new_tag);
    return ret ? EX_OK : EXIT_FAILURE;
}


/**
 * Handle clear command.
 * Removes all tags from a file.
 */
static int
handle_clear()
{
    if (!HAS_NEXT_ARG) {
        print_usage_clear(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;

    bool ret = ufa_repo_clear_tags_for_file(file);
    return ret ? EX_OK : EXIT_FAILURE;
}


static int
handle_command(char *command)
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


static int
handle_help_option(char *command)
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


int
main(int argc, char *argv[])
{
    program_name = argv[0];
    global_args  = argc;
    global_argv  = argv;

    int opt;
    char *repository = NULL;

    int error = 0;
    int r = 0, v = 0, h = 0;
    while ((opt = getopt(argc, argv, ":r:hv")) != -1) {
        switch (opt) {
        case 'r':
            if (r) {
                error = EX_USAGE;
            } else {
                r = 1;
                repository = ufa_strdup(optarg);
            }
            break;
        case 'v':
            v = 1;
            printf("%s\n", program_version);
            break;
        case 'h':
            h = 1;
            break;
        case '?':
            error = EX_USAGE;
            fprintf(stderr, "unknown option: %c\n", optopt);
            break;
        default:
            error = EX_USAGE;
        }
    }

    if (h) {
        if (HAS_NEXT_ARG) {
            return handle_help_option(NEXT_ARG);
        } else {
            print_usage(stdout);
            return EX_OK;
        }
    } else if (v) {
        return EX_OK;
    } else if (error) {
        print_usage(stderr);
        return EX_USAGE;
    }

    if (!HAS_NEXT_ARG) {
        print_usage(stderr);
        return EX_USAGE;
    }

    char *command = NEXT_ARG;
    if (repository == NULL) {
        // repository = ufa_util_get_current_dir();
        // ufa_debug("Using CWD as repository: %s", repository);
        fprintf(stderr, "error: you must specify a repository path\n");
        return EXIT_FAILURE;
    }

    ufa_error_t *err = NULL;
    if (!ufa_repo_init(repository, &err)) {
        fprintf(stderr, "%s\n", err->message);
        ufa_error_free(err);
        free(repository);
        return 1;
    }

    free(repository);

    return handle_command(command);
}
