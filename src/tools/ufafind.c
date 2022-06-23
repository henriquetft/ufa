/*
 * Copyright (c) 2022 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of ufafind command line utility.
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


/* ============================================================================================== */
/* IMPLEMENTATION                                                                                 */
/* ============================================================================================== */

static void
print_usage(FILE *stream)
{
    fprintf(stream, "\nUsage: %s [OPTIONS]\n", program_name);
    fprintf(stream, "\nCLI tool for searching files by tags and attributes\n");
    fprintf(stream,
        "\nOPTIONS\n"
        "  -h\t\tPrint this help and quit\n"
        "  -v\t\tPrint version information and quit\n"
        "  -r DIR\tRepository dir. Default is current dir\n"
        "  -a ATTRIBUTE\tFind by attribute. e.g. attribute=value\n"
        "  -t tag TAG\tFind by tag.\n"
        "  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
        "\n", program_name);
}


int
main(int argc, char *argv[])
{
    program_name = argv[0];
    global_args  = argc;
    global_argv  = argv;

    int opt;
    char *repository    = NULL;
    char *attr          = NULL;
    char *tag           = NULL;

    ufa_list_t *attrs  = NULL;
    ufa_list_t *tags   = NULL;
    ufa_list_t *result = NULL;

    int error_usage = 0;
    int exit_status = 0;
    int r = 0, v = 0, h = 0, log = 0;

    while ((opt = getopt(argc, argv, ":r:hva:t:l:")) != -1) {
        switch (opt) {
        case 'r':
            if (r) {
                error_usage = 1;
            } else {
                r = 1;
                repository = ufa_strdup(optarg);
            }
            break;
        case 'v':
            v = 1;
            break;
        case 'h':
            h = 1;
            break;
        case 'a':
            attr = ufa_strdup(optarg);
            ufa_debug("Attribute: %s\n", attr);
            
            if (strstr(attr, "=") == NULL) {
                ufa_repo_filter_attr_t *filter = ufa_repo_filter_attr_new(attr, NULL, UFA_REPO_EQUAL);
                attrs = ufa_list_append(attrs, filter);
            } else {
                ufa_list_t *parts = ufa_util_str_split(attr, "=");
                ufa_debug("Adding filter: %s=%s\n", parts->data, parts->next->data);
                ufa_repo_filter_attr_t *filter = ufa_repo_filter_attr_new(parts->data, parts->next->data, UFA_REPO_EQUAL);
                ufa_list_free_full(parts, free);
                attrs = ufa_list_append(attrs, filter);
            }
            free(attr);
            break;
        case 't':
            tag = ufa_strdup(optarg);
            ufa_debug("Adding tag: %s\n", tag);
            tags = ufa_list_append(tags, tag);
            break;
        case 'l':
            if (log) {
                error_usage = 1;
            } else {
                log = 1;
                ufa_log_level_t level = ufa_log_level_from_str(optarg);
                ufa_log_set(level);
                ufa_debug("LOG LEVEL: %s\n", optarg);
            }
            break;
        case '?':
            error_usage = 1;
            fprintf(stderr, "unknown option: %c\n", optopt);
            break;
        default:
            error_usage = 1;
        }
    }

    if (error_usage) {
        print_usage(stderr);
        exit_status = EX_USAGE;
        goto end;
    }

    if (h) {
        print_usage(stdout);
        exit_status = EX_OK;
        goto end;
    }
    if (v) {
        printf("%s\n", program_version);
        exit_status = EX_OK;
        goto end;
    }

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
        exit_status = EXIT_FAILURE;
        goto end;
    }


    result = ufa_repo_search(attrs, tags, &err);
    ufa_error_print_and_free(err);

    // print result
    for (UFA_LIST_EACH(iter_result, result)) {
        printf("%s\n", (char *) iter_result->data);
    }

end:
    free(repository);
    ufa_list_free_full(attrs, ufa_repo_filter_attr_free);
    ufa_list_free_full(tags, free);
    ufa_list_free_full(result, free);
    return exit_status;
}
