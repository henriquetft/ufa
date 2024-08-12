/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for Command-line utilities                                     */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_CLI_H_
#define UFA_CLI_H_

#include <stdio.h>
#include <getopt.h>

/* Helper macros for getopt */
#define HAS_NEXT_ARG             (optind < global_args)
#define HAS_MORE_ARGS(num)       (optind + num - 1 < global_args)
#define NEXT_ARG                 global_argv[optind++]
#define HAS_PREV_ARGS(num)       (optind - num - 2 < global_args && optind != 2)
#define PREV_ARG(num)            global_argv[optind - 2]

#define EXIT_COMMAND_NOT_FOUND   127

static char *program_name    = "";
static char *program_version = "0.1";

static int global_args    = -1;
static char **global_argv = NULL;

typedef int (*handle_command_fn_t)();
typedef void (*help_command_fn_t)(FILE *data);

int handle_command(char *command, size_t num_commands);
int handle_help_command(char *command, size_t num_commands);

#endif /* UFA_CLI_H_ */