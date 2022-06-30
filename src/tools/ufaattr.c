/*
 * Copyright (c) 2022 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of ufaattr command line utility.
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
#include "util/misc.h"

/* ============================================================================================== */
/* FUNCTIONS -- DECLARATION                                                                       */
/* ============================================================================================== */

static void
print_usage(FILE *stream);

static void
print_usage_set(FILE *stream);

static void
print_usage_unset(FILE *stream);

static void
print_usage_get(FILE *stream);

static void
print_usage_list(FILE *stream);

static void
print_usage_describe(FILE *stream);

static int
handle_set();

static int
handle_unset();

static int
handle_get();

static int
handle_list();

static int
handle_describe();

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

static const int NUM_COMMANDS = 5;
static char *commands[] = { "set", "unset", "get", "list", "describe", };

static help_command_f help_commands[] = { print_usage_set,
                                          print_usage_unset,
                                          print_usage_get,
                                          print_usage_list,
                                          print_usage_describe,
                                        };
static handle_command_f handle_commands[] = { handle_set,
                                              handle_unset,
                                              handle_get,
                                              handle_list,
                                              handle_describe,
                                            };


/* ============================================================================================== */
/* IMPLEMENTATION                                                                                 */
/* ============================================================================================== */

static void
print_usage(FILE *stream)
{
    fprintf(stream, "\nUsage: %s [OPTIONS] [COMMAND]\n", program_name);
    fprintf(stream, "\nCLI tool for managing attributes of files\n");
    fprintf(stream,
        "\nOPTIONS\n"
        "  -h\t\tPrint this help and quit\n"
        "  -v\t\tPrint version information and quit\n"
        "  -r DIR\tRepository dir. Default is current dir\n"
        "  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
        "\n"
        "COMMANDS\n"
        "  set\t\tSet attributes on file\n"
        "  unset\t\tUnset attributes on file\n"
        "  get\t\tGet the value of an attribute\n"
        "  list\t\tList attributes of a file\n"
        "  describe\t\tList attributes and values of a file\n"
        "\n"
        "Run '%s COMMAND -h' for more information on a command.\n"
        "\n", program_name);
}


static void
print_usage_set(FILE *stream)
{
    printf("\nUsage:  %s set FILE ATTRIBUTE VALUE\n", program_name);
    printf("\nSet attributes on file\n\n");
}

static void
print_usage_unset(FILE *stream)
{
    printf("\nUsage:  %s unset FILE ATTRIBUTE\n", program_name);
    printf("\nUnset attributes on file\n\n");
}

static void
print_usage_get(FILE *stream)
{
    printf("\nUsage:  %s get FILE ATTRIBUTE\n", program_name);
    printf("\nGet the value of an attribute\n\n");
}

static void
print_usage_list(FILE *stream)
{
    printf("\nUsage:  %s list FILE\n", program_name);
    printf("\nList attributes of a file\n\n");
}

static void
print_usage_describe(FILE *stream)
{
    printf("\nUsage:  %s describe FILE\n", program_name);
    printf("\nList attributes and values of a file\n\n");
}


static int
handle_set()
{
    if (!HAS_MORE_ARGS(3)) {
        print_usage_set(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;
    char *attr = NEXT_ARG;
    char *value = NEXT_ARG;

    ufa_error_t *error = NULL;
    bool is_ok         = ufa_repo_set_attr(file, attr, value, &error);
    ufa_error_print_and_free(error);
    return is_ok ? EX_OK : EXIT_FAILURE;
}


static int
handle_unset()
{
    if (!HAS_MORE_ARGS(2)) {
        print_usage_unset(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;
    char *attr = NEXT_ARG;

    ufa_error_t *error = NULL;
    bool is_ok         = ufa_repo_unset_attr(file, attr, &error);
    ufa_error_print_and_free(error);
    return is_ok ? EX_OK : EXIT_FAILURE;
}


static int
handle_get()
{
    if (!HAS_MORE_ARGS(2)) {
        print_usage_get(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;
    char *attr = NEXT_ARG;

    bool found = false;

    ufa_error_t *error = NULL;
    ufa_list_t *list_attrs = ufa_repo_get_attr(file, &error);
    ufa_error_print_and_free(error);
    for (UFA_LIST_EACH(iter_attr, list_attrs)) {
        ufa_repo_attr_t *attr_iter = (ufa_repo_attr_t *) iter_attr->data;
        if (ufa_util_strequals(attr_iter->attribute, attr)) {
            printf("%s\n", attr_iter->value);
            found = true;
        }
    }
    ufa_list_free_full(list_attrs, ufa_repo_attr_free);
    return (!error && found) ? EX_OK : EXIT_FAILURE;
}


static int
handle_list()
{
    if (!HAS_MORE_ARGS(1)) {
        print_usage_list(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;
    char *attr = NEXT_ARG;

    ufa_error_t *error = NULL;
    ufa_list_t *list_attrs = ufa_repo_get_attr(file, &error);
    ufa_error_print_and_free(error);
    for (UFA_LIST_EACH(iter_attr, list_attrs)) {
        printf("%s\n", ((ufa_repo_attr_t *) iter_attr->data)->attribute);
    }
    ufa_list_free_full(list_attrs, ufa_repo_attr_free);
    return !error ? EX_OK : EXIT_FAILURE;
}

static int
handle_describe()
{
    if (!HAS_MORE_ARGS(1)) {
        print_usage_describe(stderr);
        return EX_USAGE;
    }

    char *file = NEXT_ARG;

    ufa_error_t *error = NULL;
    ufa_list_t *list_attrs = ufa_repo_get_attr(file, &error);
    ufa_error_print_and_free(error);
    for (UFA_LIST_EACH(iter_attr, list_attrs)) {
        ufa_repo_attr_t *attr_iter = (ufa_repo_attr_t *) iter_attr->data;
        printf("%s %s\n", (char *) attr_iter->attribute, (char *) attr_iter->value);
    }
    ufa_list_free_full(list_attrs, ufa_repo_attr_free);
    return !error ? EX_OK : EXIT_FAILURE;
}

static int
handle_command(char *command)
{
    int exit_status = EXIT_COMMAND_NOT_FOUND;
    for (int pos = 0; pos < NUM_COMMANDS; pos++) {
        if (ufa_util_strequals(command, commands[pos])) {
            ufa_debug("executing command '%s'", command);
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
    char *repository    = NULL;

    bool error_usage = false;
    int exit_status = EX_OK;
    int r = 0, log = 0;

    while ((opt = getopt(argc, argv, ":r:l:hv")) != -1 && !error_usage) {
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
            print_usage(stdout);
            exit_status = EX_OK;
            goto end;
        case 'l':
            if (log) {
                error_usage = true;
            } else {
                log = 1;
                ufa_log_level_t level = ufa_log_level_from_str(optarg);
                ufa_log_set(level);
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

    ufa_error_t *err = NULL;
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