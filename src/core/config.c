/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* UFA configuration mechanism (implementation of config.h).                  */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#include "core/config.h"
#include "core/errors.h"
#include "util/logging.h"
#include "util/misc.h"
#include <pthread.h>
#include <stdio.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static struct ufa_list *dir_list = NULL;
static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */


static char *get_config_dirs_filepath()
{
	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	char *dirs_file = ufa_util_joinpath(cfg_dir, DIRS_FILE_NAME, NULL);
	ufa_free(cfg_dir);
	return dirs_file;
}

static bool check_and_create_config_dir(struct ufa_error **error)
{
	bool ret = false;
	char *dirs_file = NULL;
	char *cfg_dir = NULL;
	FILE *out = NULL;

	cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);

	if (!ufa_util_isdir(cfg_dir)) {
		char *base_cfg_dir = ufa_util_config_dir(NULL);
		if (ufa_util_isdir(base_cfg_dir)) {
			ufa_debug("Creating dir '%s'", cfg_dir);
			bool created = ufa_util_mkdir(cfg_dir, error);
			if (!created) {
				goto end;
			}
		} else {
			ufa_error_new(error, UFA_ERROR_NOTDIR,
				      "Base config dir does not exist: %s",
				      base_cfg_dir);
			goto end;
		}
	}

	dirs_file = ufa_util_joinpath(cfg_dir, DIRS_FILE_NAME, NULL);
	if (!ufa_util_isfile(dirs_file)) {
		ufa_debug("Creating '%s'", dirs_file);
		out = fopen(dirs_file, "w");
		if (out == NULL) {
			ufa_error_new(error, UFA_ERROR_FILE,
				      "Could not open file: %s", dirs_file);
			goto end;
		}
		fprintf(out, DIRS_FILE_DEFAULT_STRING);
	}

	ret = true;

end:
	if (out) {
		fclose(out);
	}
	ufa_free(dirs_file);
	ufa_free(cfg_dir);
	return ret;
}

static void write_config(const struct ufa_list *list, struct ufa_error **error)
{
	ufa_return_if(list == NULL);

	char *dirs_file = NULL;
	FILE *file = NULL;

	dirs_file = get_config_dirs_filepath();
	file = fopen(dirs_file, "w");
	if (file == NULL) {
		ufa_error_new(error, UFA_ERROR_FILE, "Could not open file: %s",
			      file);
		goto end;
	}

	fprintf(file, DIRS_FILE_DEFAULT_STRING);

	for (UFA_LIST_EACH(i, list)) {
		char *dir = (char *)i->data;
		fprintf(file, "%s\n", dir);
	}
end:
	if (file != NULL) {
		fclose(file);
	}
	ufa_free(dirs_file);
}

/* ========================================================================== */
/* FUNCTIONS FROM config.h                                                    */
/* ========================================================================== */

struct ufa_list *ufa_config_dirs(bool reload, struct ufa_error **error)
{
	pthread_mutex_lock(&mutex);

	struct ufa_list *list = NULL;
	const size_t MAX_LINE = 1024;
	char linebuf[MAX_LINE];
	char *dirs_file = NULL;
	FILE *file = NULL;

	if (dir_list && !reload) {
		goto end;
	}

	if (!check_and_create_config_dir(error)) {
		goto end;
	}

	dirs_file = get_config_dirs_filepath();

	ufa_info("Reading config file %s", dirs_file);
	file = fopen(dirs_file, "r");
	if (file == NULL) {
		// FIXME create ufa_error
		perror("fopen");
		goto end;
	}

	while (fgets(linebuf, MAX_LINE, file)) {
		char *line = ufa_str_trim(linebuf);
		if (!ufa_str_startswith(line, "#") &&
		    !ufa_str_equals(line, "")) {
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
	dir_list = list;
	pthread_mutex_unlock(&mutex);

	// FIXME return a copy?
	return list;
}

bool ufa_config_add_dir(const char *dir, struct ufa_error **error)
{
	FILE *file = NULL;
	bool ret = false;
	char *normdir = NULL;
	char *dirs_file = NULL;
	struct ufa_list *dirs = NULL;

	pthread_mutex_lock(&mutex);

	dirs = ufa_config_dirs(true, error);

	if_goto(HAS_ERROR(error), end);

	normdir = ufa_util_abspath(dir);

	bool found_in_list = ufa_list_contains(
	    dirs, normdir, (bool (*)(void *, void *))ufa_str_equals);

	ufa_debug("Dir '%s' already in list: %d\n", normdir, found_in_list);

	if (!found_in_list) {
		ufa_debug("Adding dir to config: %s", normdir);
		ufa_list_append(dirs, ufa_strdup(normdir));
		write_config(dirs, error);
	}

end:
	ufa_free(normdir);
	ufa_list_free_full(dirs, ufa_free);
	pthread_mutex_unlock(&mutex);
	return ret;
}

bool ufa_config_remove_dir(const char *dir, struct ufa_error **error)
{
	char *dirs_file = NULL;
	bool ret = false;
	char *normdir = NULL;
	struct ufa_list *dirs = NULL;

	pthread_mutex_lock(&mutex);

	dirs = ufa_config_dirs(true, error);
	if_goto(HAS_ERROR(error), end);

	dirs_file = get_config_dirs_filepath();
	ufa_info("Reading config file %s", dirs_file);

	normdir = ufa_util_abspath(dir);

	struct ufa_list *element = ufa_list_find_by_data(
	    dirs, normdir, (bool (*)(void *, void *))ufa_str_equals);

	if (element == NULL) {
		ufa_debug("Dir '%s' was not in the list\n", normdir);
		ret = true;
		goto end;
	}

	dirs = ufa_list_unlink_node(dirs, element);
	ufa_list_free_full(element, ufa_free);
	dir_list = dirs;
	write_config(dirs, error);
	if_goto(HAS_ERROR(error), end);

	ret = true;
end:
	pthread_mutex_unlock(&mutex);
	ufa_free(normdir);
	ufa_free(dirs_file);
	return ret;
}