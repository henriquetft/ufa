/*
* Copyright (c) 2022 Henrique Teófilo
* All rights reserved.
*
* Implementation of UFA Daemon.
*
* For the terms of usage and distribution, please see COPYING file.
 */

#include "core/monitor.h"
#include "util/misc.h"
#include "util/logging.h"
#include <stdio.h>

#define   CONFIG_DIR_NAME           "ufa"
#define   DIRS_FILE_NAME            "dirs"
#define   DIRS_FILE_DEFAULT_STRING  "# UFA repository folders\n\n"


static void print_event(const struct ufa_event *event)
{
	char str[100] = "";
	ufa_event_tostr(event->event, str, 100);
	printf("========== NEW EVENT ========== \n");
	if (str)
		printf("EVENT......: %s\n", str);
	if (event->target1)
		printf("TARGET1....: %s\n", event->target1);
	if (event->target2)
		printf("TARGET2....: %s\n", event->target2);

	printf("WATCHER1...: %d\n", event->watcher1);
	printf("WATCHER2...: %d\n", event->watcher2);
}

void callback_event_repo(const struct ufa_event *event)
{
	// handle .goutputstream-* files
	print_event(event);
}

void callback_event_config(const struct ufa_event *event)
{
	if (event->event == UFA_MONITOR_CLOSEWRITE && ufa_str_endswith(event->target1, DIRS_FILE_NAME)) {
		printf("changing config file\n");
	}
}

static int check_and_create_config_dir()
{
	int ret = 0;
	char *dirs_file = NULL;
	char *cfg_dir = NULL;
	FILE *out = NULL;

	cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);

	if (!ufa_util_isdir(cfg_dir)) {
		char *base_cfg_dir = ufa_util_config_dir(NULL);
		if (ufa_util_isdir(base_cfg_dir)) {
			ufa_debug("Creating dir '%s'", cfg_dir);
			struct ufa_error *error;
			bool created = ufa_util_mkdir(cfg_dir, &error);
			if (!created) {
				ufa_error_print_and_free_prefix(
				    error, "error creating base dir");
				ret = -1;
				goto end;
			}
		} else {
			fprintf(stderr, "Base config dir does not exist: %s\n",
				base_cfg_dir);
			ret = -1;
			goto end;
		}
	}

	dirs_file = ufa_util_joinpath(cfg_dir, DIRS_FILE_NAME, NULL);
	if (!ufa_util_isfile(dirs_file)) {
		ufa_debug("Creating '%s'", dirs_file);
		out = fopen(dirs_file, "w");
		if (out == NULL) {
			fprintf(stderr, "could not open file: %s\n", dirs_file);
			ret = -1;
			goto end;
		}
		fprintf(out, DIRS_FILE_DEFAULT_STRING);
	}

	ret = 0;

end:
	if (out) {
		fclose(out);
	}
	free(dirs_file);
	free(cfg_dir);
	return ret;
}


static struct ufa_list *get_valid_dirs()
{
	struct ufa_list *list = NULL;
	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	char *dirs_file = ufa_util_joinpath(cfg_dir, DIRS_FILE_NAME, NULL);

	ufa_debug("Reading config file %s", dirs_file);
	FILE *f = fopen(dirs_file, "r");
	size_t max_line = 1024;
	char linebuf[max_line];
	while (fgets(linebuf, max_line, f)) {
		char *line = ufa_str_trim(linebuf);
		if (!ufa_str_startswith(line, "#") &&
		    !ufa_util_strequals(line, "")) {
			if (!ufa_util_isdir(line)) {
				ufa_debug("%s is not a dir", line);
			} else {
				ufa_debug("%s is a valid dir", line);
				list = ufa_list_append(list, ufa_strdup(line));
			}
		}
	}
	free(cfg_dir);
	return list;
}

int main(int argc, char *argv[])
{
	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	ufa_info("Config dir: %s\n", cfg_dir);

	int ret = check_and_create_config_dir();
	if (ret) {
		exit(ret);
	}

	struct ufa_list *dirs = get_valid_dirs();

	ufa_monitor_init(); // FIXME check error

	ufa_debug("Adding watcher to config dir: %s", cfg_dir);
	ufa_monitor_add_watcher(cfg_dir,
				UFA_MONITOR_MOVE | UFA_MONITOR_DELETE |
				    UFA_MONITOR_CLOSEWRITE,
				callback_event_config);

	ufa_debug("Adding watcher to repo dirs");
	for (UFA_LIST_EACH(i, dirs)) {
		ufa_debug("Adding watcher to: %s", i->data);
		ufa_monitor_add_watcher(i->data,
					UFA_MONITOR_MOVE | UFA_MONITOR_DELETE |
					    UFA_MONITOR_CLOSEWRITE,
					callback_event_repo);
	}

	printf("Waiting...\n");
	while (1)
		;

	printf("Terminating %s\n", argv[0]);
}