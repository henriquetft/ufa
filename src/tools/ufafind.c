/* ========================================================================== */
/* Copyright (c) 2022 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of ufafind command line utility.                            */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/repo.h"
#include "tools/cli.h"
#include "util/list.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/string.h"
#include "json/jsonrpc_api.h"
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static char *match_mode_str[] = {"=", "~="};

/* ========================================================================== */
/* IMPLEMENTATION                                                             */
/* ========================================================================== */

static void print_usage(FILE *stream)
{
	fprintf(stream, "\nUsage: %s [OPTIONS]\n", program_name);
	fprintf(stream,
		"\nCLI tool for searching files by tags and attributes\n");
	fprintf(stream,
		"\nOPTIONS\n"
		"  -h\t\tPrint this help and quit\n"
		"  -v\t\tPrint version information and quit\n"
		"  -r DIR\tRepository dir. Default is current dir + list of "
		"dirs on config file \n"
		"  -a ATTRIBUTE\tFind by attribute. e.g. attribute=value\n"
		"  -t tag TAG\tFind by tag\n"
		"  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
		"\n");
}

static void _add_attr(char *optarg, struct ufa_list **attrs)
{
	char *attr = ufa_str_dup(optarg);
	int mm_index = -1;

	ufa_debug("Attribute: %s", attr);

	// Finding out which matchmode was used
	// finding by simple string comparison, so using matchmode with most
	// characters e.g. "=" is in both matchmodes "=" and "!="
	for (int x = 0; x < UFA_REPO_MATCHMODE_TOTAL; x++) {
		char *str_mm = match_mode_str[x];
		if (strstr(attr, str_mm) != NULL) {
			if (mm_index == -1) {
				mm_index = x;
			} else if (strlen(match_mode_str[x]) >
				   strlen(match_mode_str[mm_index])) {
				mm_index = x;
			}
		}
	}

	if (mm_index != -1) {
		char *str_mm = match_mode_str[mm_index];
		enum ufa_repo_matchmode mm =
		    ufa_repo_matchmode_supported[mm_index];
		struct ufa_list *parts = ufa_str_split(attr, str_mm);
		ufa_debug("Adding filter: %s / %s (matchmode: %s)",
			  parts->data, parts->next->data, str_mm);
		struct ufa_repo_filterattr *filter =
		    ufa_repo_filterattr_new(parts->data, parts->next->data, mm);
		ufa_list_free(parts);
		*attrs = ufa_list_append2(*attrs, filter,
			(ufa_list_free_fn_t) ufa_repo_filterattr_free);
	} else {
		// When the user specify the attribute with no matchmode
		// Used just to check presence of an attribute
		struct ufa_repo_filterattr *filter =
		    ufa_repo_filterattr_new(attr, NULL, UFA_REPO_EQUAL);
		*attrs = ufa_list_append2(*attrs, filter,
			(ufa_list_free_fn_t) ufa_repo_filterattr_free);
	}
	ufa_free(attr);
}

int main(int argc, char *argv[])
{
	program_name = argv[0];
	global_args  = argc;
	global_argv  = argv;

	char *repository = NULL;
	char *cwd        = NULL;
	char *tag        = NULL;

	struct ufa_list *attrs     = NULL;
	struct ufa_list *tags      = NULL;
	struct ufa_list *result    = NULL;
	struct ufa_list *list_dirs = NULL;
	struct ufa_error *err_api  = NULL;

	bool error_usage = false;
	int exit_status = EX_OK;
	int r = 0, log = 0;

	int opt;
	while ((opt = getopt(argc, argv, ":r:hva:t:l:")) != -1 &&
	       !error_usage) {
		switch (opt) {
		case 'r':
			if (r) {
				error_usage = true;
			} else {
				r = 1;
				repository = ufa_str_dup(optarg);
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
			tag = ufa_str_dup(optarg);
			ufa_debug("Adding tag: %s", tag);
			tags = ufa_list_append(tags, tag);
			break;
		case 'l':
			if (log) {
				error_usage = true;
			} else {
				log = 1;
				enum ufa_log_level level =
				    ufa_log_level_from_str(optarg);
				ufa_log_setlevel(level);
				ufa_debug("LOG LEVEL: %s", optarg);
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

	if (error_usage || (tags == NULL && attrs == NULL)) {
		print_usage(stderr);
		exit_status = EX_USAGE;
		goto end;
	}

	// Start JSON RPC API
	ufa_jsonrpc_api_t *api = ufa_jsonrpc_api_init(&err_api);
	ufa_error_exit(err_api, EX_UNAVAILABLE);

	if (repository != NULL) {
		list_dirs = ufa_list_append(list_dirs,
					    ufa_util_abspath(repository));
		result = ufa_jsonrpc_api_search(api,
						list_dirs,
						attrs,
						tags,
						false,
						&err_api);
	} else {
		cwd = ufa_util_get_current_dir();
		if (ufa_repo_isrepo(cwd)) {
			list_dirs = ufa_list_append(list_dirs, ufa_str_dup(cwd));
		}
		result = ufa_jsonrpc_api_search(api,
						list_dirs,
						attrs,
						tags,
						true,
						&err_api);
	}

	if (err_api) {
		ufa_error_print(err_api);
		exit_status = 1;
		goto end;
	}

	// Print result
	for (UFA_LIST_EACH(i, result)) {
		printf("%s\n", (char *) i->data);
	}

end:
	ufa_error_free(err_api);
	ufa_free(repository);
	ufa_free(cwd);
	ufa_list_free_full(list_dirs, ufa_free);
	ufa_list_free_full(attrs,
			   (ufa_list_free_fn_t) ufa_repo_filterattr_free);
	ufa_list_free_full(tags, ufa_free);
	ufa_list_free(result);
	ufa_jsonrpc_api_close(api, NULL);

	return exit_status;
}
