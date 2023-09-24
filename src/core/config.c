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
#include "util/string.h"
#include <pthread.h>
#include <stdio.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static struct ufa_list *global_dirlist = NULL;
static pthread_mutex_t global_mutex    = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static char *get_config_dirs_filepath();
static bool check_and_create_config_dir(struct ufa_error **error);
static void write_config(const struct ufa_list *list, struct ufa_error **error);


/* ========================================================================== */
/* FUNCTIONS FROM config.h                                                    */
/* ========================================================================== */

struct ufa_list *ufa_config_dirs(bool reload, struct ufa_error **error)
{
	pthread_mutex_lock(&global_mutex);

	struct ufa_list *list = NULL;
	const size_t MAX_LINE = 1024;
	char linebuf[MAX_LINE];
	char *dirs_file = NULL;
	FILE *file = NULL;

	if (global_dirlist && !reload) {
		list = global_dirlist;
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
				list = ufa_list_prepend(list,
							ufa_str_dup(line));
			}
		}
	}

	list = ufa_list_reverse(list);

end:
	if (file) {
		fclose(file);
	}
	ufa_free(dirs_file);
	global_dirlist = list;
	pthread_mutex_unlock(&global_mutex);

	struct ufa_list *ret = ufa_list_clone(
	    global_dirlist, (ufa_list_cpydata_fn_t) ufa_str_dup, ufa_free);
	return ret;
}

bool ufa_config_add_dir(const char *dir, struct ufa_error **error)
{
	bool ret = false;
	char *normdir = NULL;
	struct ufa_list *dirs = NULL;

	pthread_mutex_lock(&global_mutex);

	dirs = ufa_config_dirs(true, error);

	if_goto(HAS_ERROR(error), end);

	normdir = ufa_util_abspath(dir);
	bool isdir = ufa_util_isdir(normdir);
	ufa_debug("DIR abspath: (%s)", normdir);
	ufa_debug("IS DIR: %d", isdir);
	if (!isdir) {
		ufa_error_new(error, UFA_ERROR_FILE, "'%s' is not a dir", dir);
		goto end;
	}

	bool found_in_list = ufa_list_contains(dirs, normdir, ufa_str_equals);

	ufa_debug("Dir '%s' found in list? %d\n", normdir, found_in_list);

	if (!found_in_list) {
		ufa_debug("Adding dir to config: %s", normdir);
		dirs = ufa_list_append2(dirs, ufa_str_dup(normdir), ufa_free);
		write_config(dirs, error);
		if_goto(HAS_ERROR(error), end);
	}

	ret = true;
end:
	ufa_free(normdir);
	ufa_list_free(dirs);
	pthread_mutex_unlock(&global_mutex);
	return ret;
}

bool ufa_config_remove_dir(const char *dir, struct ufa_error **error)
{
	ufa_debug("Removing dir '%s'", dir);

	char *dirs_file = NULL;
	bool ret = false;
	struct ufa_list *dirs = NULL;

	pthread_mutex_lock(&global_mutex);

	dirs = ufa_config_dirs(true, error);
	if_goto(HAS_ERROR(error), end);

	dirs_file = get_config_dirs_filepath();
	ufa_info("Reading config file %s", dirs_file);

	struct ufa_list *element = ufa_list_find_by_data(
	    dirs, dir, (ufa_list_equal_fn_t) ufa_str_equals);

	if (element == NULL) {
		ufa_debug("Dir '%s' was not in the list\n", dir);
		ret = true;
		goto end;
	}

	dirs = ufa_list_unlink_node(dirs, element);
	ufa_list_free(element);
	global_dirlist = dirs;
	write_config(dirs, error);
	if_goto(HAS_ERROR(error), end);

	ret = true;
end:
	pthread_mutex_unlock(&global_mutex);
	ufa_free(dirs_file);
	return ret;
}


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
	char *base_cfg_dir = NULL;

	cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);

	if (!ufa_util_isdir(cfg_dir)) {
		base_cfg_dir = ufa_util_config_dir(NULL);
		if (ufa_util_isdir(base_cfg_dir)) {
			ufa_debug("Creating dir '%s'", cfg_dir);
			bool created = ufa_util_mkdir(cfg_dir, error);
			if (!created) {
				goto end;
			}
		} else {
			ufa_error_new(error,
				      UFA_ERROR_NOTDIR,
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
			ufa_error_new(error,
				      UFA_ERROR_FILE,
				      "Could not open file: %s",
				      dirs_file);
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
	ufa_free(base_cfg_dir);
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

	if (ufa_log_is_logging(UFA_LOG_DEBUG)) {
		ufa_debug("Writing %d dirs to '%s'", ufa_list_size(list),
			  dirs_file);
	}
	for (UFA_LIST_EACH(i, list)) {
		char *dir = (char *) i->data;
		ufa_debug("Writing '%s' to file '%s'", dir, dirs_file);
		fprintf(file, "%s\n", dir);
	}
end:
	if (file != NULL) {
		fclose(file);
	}
	ufa_free(dirs_file);
}
