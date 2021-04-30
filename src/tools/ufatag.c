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
handle_help_command(char *command);


/* ============================================================================================== */
/* VARIABLES AND DEFINITIONS                                                                      */
/* ============================================================================================== */

#define HAS_NEXT_ARG (optind < global_args)
#define HAS_MORE_ARGS(num) (optind+num-1 < global_args)
#define NEXT_ARG global_argv[optind++]

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
        return -1;
    }

    char *file = NEXT_ARG;
    char *tag = NEXT_ARG;

    int ret = ufa_repo_unset_tag_on_file(file, tag);
    return ret ? 0 : 1;
}


/* print all tags of a file */
static int
handle_list()
{
    if (!HAS_NEXT_ARG) {
        print_usage_list(stderr);
        return -1;
    }

    char *arg = NEXT_ARG;


    /* FIXME must fail if file does not exist */
    ufa_list_t *list = NULL;
    int ret = ufa_repo_get_tags_for_file(arg, &list);

    for (ufa_list_t *iter = list; iter != NULL; iter = iter->next) {
        printf("%s\n", (char *)iter->data);
    }
    ufa_list_free_full(list, free);


    return ret >= 0 ? 0 : 1;
}

/* set tag on file */
static int
handle_set()
{
    if (!HAS_MORE_ARGS(2)) {
        print_usage_set(stderr);
        return -1;
    }

    char *file = NEXT_ARG;
    char *new_tag = NEXT_ARG;

    int ret = ufa_repo_set_tag_on_file(file, new_tag);
    return ret ? 0 : 1;
}


/* remove all tags from a file */
static int
handle_clear()
{
    if (!HAS_NEXT_ARG) {
        print_usage_clear(stderr);
        return -1;
    }

    char *file = NEXT_ARG;

    int ret = ufa_repo_clear_tags_for_file(file);
    return ret ? 0 : 1;
}



static int
handle_command(char *command)
{
    int exit_status = -1;
    for (int pos = 0; pos < NUM_COMMANDS; pos++) {
        if (ufa_util_strequals(command, commands[pos])) {
            exit_status = handle_commands[pos]();
        }
    }
    if (exit_status == -1) {
        fprintf(stderr, "\ninvalid command");
        fprintf(stderr, "\nSee %s -h\n", program_name);
        exit_status = 1;
    }
    return exit_status;
}

static int
handle_help_command(char *command)
{
    int exit_code = -1;
    for (int pos = 0; pos < NUM_COMMANDS; pos++) {
        if (ufa_util_strequals(command, commands[pos])) {
            help_commands[pos](stdout);
            exit_code = 0;
        }
    }
    if (exit_code != 0) {
        fprintf(stderr, "invalid command\n");
    }
    return exit_code;
}


int
main(int argc, char *argv[])
{
    program_name = argv[0];
    global_args = argc;
    global_argv = argv;

    int opt;
    char *repository = NULL;

    int error = 0;
    int r = 0, v = 0, h = 0;
    while ((opt = getopt(argc, argv, ":r:hv")) != -1) {
        switch (opt) {
        case 'r':
            if (r) {
                error = 1;
            } else {
                r = 1;
                repository = optarg;
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
            error = 1;
            fprintf(stderr, "unknown option: %c\n", optopt);
            break;
        default:
            error = 1;
        }
    }

    if (h) {
        if (HAS_NEXT_ARG) {
            return handle_help_command(NEXT_ARG);
        } else {
            print_usage(stdout);
            return 0;
        }
    } else if (v) {
        return 0;
    } else if (error == 1) {
        print_usage(stderr);
        return -1;
    }

    if (!HAS_NEXT_ARG) {
        print_usage(stderr);
        return -1;
    }

    char *command = NEXT_ARG;
    ufa_repo_init(repository);
    return handle_command(command);
}
