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


int
main(int argc, char *argv[])
{
    program_name = argv[0];
    global_args  = argc;
    global_argv  = argv;

    int opt;
    char *repository    = NULL;
}