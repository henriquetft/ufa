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

static char *match_mode_str[] = { "=", "~=" };


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
        "  -t tag TAG\tFind by tag\n"
        "  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
        "\n");
}


static void
_add_attr(char *optarg, ufa_list_t **attrs)
{
    char *attr = ufa_strdup(optarg);
    ufa_debug("Attribute: %s", attr);
    int mm_index = -1;
    // finding out which matchmode was used
    // finding by simple string comparison, so using matchmode with most characters
    // e.g. "=" is in both matchmodes "=" and "!="
    for (int x = 0; x < UFA_REPO_MATCH_MODE_TOTAL; x++) {
        char *str_mm = match_mode_str[x];
        enum ufa_repo_match_mode match_mode = ufa_repo_match_mode_supported[x];            
        if (strstr(attr, str_mm) != NULL) {
            if (mm_index == -1) {
                mm_index = x;
            } else if (strlen(match_mode_str[x]) > strlen(match_mode_str[mm_index])) {
                mm_index = x;
            }
        }
    }

    if (mm_index != -1) {
        char *str_mm = match_mode_str[mm_index];
        enum ufa_repo_match_mode mm = ufa_repo_match_mode_supported[mm_index];            
        ufa_list_t *parts = ufa_util_str_split(attr, str_mm);
        ufa_debug("Adding filter: %s / %s (matchmode: %s)\n", parts->data, parts->next->data, str_mm);
        struct ufa_repo_filter_attr *filter = ufa_repo_filter_attr_new(parts->data, parts->next->data, mm);
        ufa_list_free_full(parts, free);
        *attrs = ufa_list_append(*attrs, filter);
    } else {
        struct ufa_repo_filter_attr *filter = ufa_repo_filter_attr_new(attr, NULL, UFA_REPO_EQUAL);
        *attrs = ufa_list_append(*attrs, filter);
    }
    free(attr);
}


int
main(int argc, char *argv[])
{
    program_name = argv[0];
    global_args  = argc;
    global_argv  = argv;

    int opt;
    char *repository   = NULL;
    char *tag          = NULL;

    ufa_list_t *attrs  = NULL;
    ufa_list_t *tags   = NULL;
    ufa_list_t *result = NULL;

    bool error_usage = false;
    int exit_status = EX_OK;
    int r = 0, log = 0;

    while ((opt = getopt(argc, argv, ":r:hva:t:l:")) != -1 && !error_usage) { 
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
        case 'a':
            _add_attr(optarg, &attrs);
            break;
        case 't':
            tag = ufa_strdup(optarg);
            ufa_debug("Adding tag: %s\n", tag);
            tags = ufa_list_append(tags, tag);
            break;
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
        exit_status = EXIT_FAILURE;
        goto end;
    }


    result = ufa_repo_search(attrs, tags, &err);
    if (err) {
        ufa_error_print(err);
        exit_status = 1;
    }

    // print result
    for (UFA_LIST_EACH(iter_result, result)) {
        printf("%s\n", (char *) iter_result->data);
    }

end:
    ufa_error_free(err);
    free(repository);
    ufa_list_free_full(attrs, ufa_repo_filter_attr_free);
    ufa_list_free_full(tags, free);
    ufa_list_free_full(result, free);
    return exit_status;
}
