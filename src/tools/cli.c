/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Command-line utilities.                                                    */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#include "tools/cli.h"
#include "util/misc.h"
#include "util/logging.h"
#include <stdio.h>
#include <sysexits.h>


extern char *commands[];
extern help_command_f help_commands[];
extern handle_command_f handle_commands[];

int handle_command(char *command, size_t num_commands)
{
	int exit_status = EXIT_COMMAND_NOT_FOUND;
	for (size_t i = 0; i < num_commands; i++) {
		if (ufa_str_equals(command, commands[i])) {
			ufa_debug("Executing command '%s'", command);
			exit_status = handle_commands[i]();
		}
	}
	if (exit_status == EXIT_COMMAND_NOT_FOUND) {
		fprintf(stderr, "\nInvalid command");
		fprintf(stderr, "\nSee %s -h\n", program_name);
	}
	return exit_status;
}

int handle_help_command(char *command, size_t num_commands)
{
	int exit_code = EXIT_COMMAND_NOT_FOUND;
	for (size_t i = 0; i < num_commands; i++) {
		if (ufa_str_equals(command, commands[i])) {
			help_commands[i](stdout);
			exit_code = EX_OK;
		}
	}
	if (exit_code != EX_OK) {
		fprintf(stderr, "Invalid command\n");
	}
	return exit_code;
}