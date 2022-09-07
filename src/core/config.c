/*
* Copyright (c) 2022 Henrique Teófilo
* All rights reserved.
*
* Implementation of config functions (config.h).
*
* For the terms of usage and distribution, please see COPYING file.
*/

#include "core/config.h"
#include "util/misc.h"
#include "util/logging.h"
#include <stdio.h>
#include <pthread.h>

static struct ufa_list *dir_list = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


struct ufa_list *ufa_config_dirs(bool reload)
{
	pthread_mutex_lock(&mutex);

	const size_t MAX_LINE = 1024;
	char linebuf[MAX_LINE];
	struct ufa_list *list = NULL;
	char *cfg_dir = NULL;
	char *dirs_file = NULL;
	FILE *file = NULL;

	if (dir_list && !reload) {
		goto end;
	}

	dir_list = NULL;
	cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	dirs_file = ufa_util_joinpath(cfg_dir, DIRS_FILE_NAME, NULL);


	ufa_info("Reading config file %s", dirs_file);
	file = fopen(dirs_file, "r");
	if (file == NULL) {
		perror("fopen");
		goto end;
	}

	while (fgets(linebuf, MAX_LINE, file)) {
		char *line = ufa_str_trim(linebuf);
		if (!ufa_str_startswith(line, "#") &&
		    !ufa_util_strequals(line, "")) {
			if (!ufa_util_isdir(line)) {
				ufa_warn("%s is not a dir", line);
			} else {
				ufa_debug("%s is a valid dir", line);
				list = ufa_list_append(list, ufa_strdup(line));
			}
		}
	}

end:
	if (file) {
		fclose(file);
	}
	ufa_free(dirs_file);
	ufa_free(cfg_dir);
	if (list) {
		dir_list = list;
	}
	pthread_mutex_unlock(&mutex);
	return dir_list;
}