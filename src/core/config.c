/* ========================================================================== */
/* Copyright (c) 2022-2025 Henrique Teófilo                                   */
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
#include <unistd.h>

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
	struct ufa_list *ret  = NULL;
	struct ufa_list *list = NULL;
	const size_t MAX_LINE = 1024;
	char *dirs_file       = NULL;
	FILE *file            = NULL;

	char linebuf[MAX_LINE];

	ufa_goto_iferror(error, end);

	pthread_mutex_lock(&global_mutex);

	if (global_dirlist && !reload) {
		list = global_dirlist;
		goto freeres;
	}

	if (!check_and_create_config_dir(error)) {
		goto freeres;
	}

	dirs_file = get_config_dirs_filepath();

	ufa_info("Reading config file %s", dirs_file);
	file = fopen(dirs_file, "r");
	if (file == NULL) {
		// FIXME create ufa_error
		perror("fopen");
		goto freeres;
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

freeres:
	if (file) {
		fclose(file);
	}
	ufa_free(dirs_file);
	global_dirlist = list;
	pthread_mutex_unlock(&global_mutex);

	ret = ufa_list_clone(
	    global_dirlist, (ufa_list_cpydata_fn_t) ufa_str_dup, ufa_free);
end:
	return ret;
}

bool ufa_config_add_dir(const char *dir, struct ufa_error **error)
{
	bool ret              = false;
	char *normdir         = NULL;
	struct ufa_list *dirs = NULL;

	ufa_goto_iferror(error, freeres);

	pthread_mutex_lock(&global_mutex);

	dirs = ufa_config_dirs(true, error);

	ufa_goto_iferror(error, end);

	normdir = ufa_util_abspath(dir);
	bool isdir = ufa_util_isdir(normdir);
	ufa_debug("DIR abspath: (%s)", normdir);
	ufa_debug("IS DIR: %d", isdir);
	if (!isdir) {
		ufa_error_new(error, UFA_ERROR_NOTDIR,
		              "'%s' is not a dir",
		              dir);
		goto end;
	}

	bool found_in_list = ufa_list_contains(dirs, normdir, ufa_str_equals);

	ufa_debug("Dir '%s' found in list? %d\n", normdir, found_in_list);

	if (!found_in_list) {
		ufa_debug("Adding dir to config: %s", normdir);
		dirs = ufa_list_append2(dirs, ufa_str_dup(normdir),
		                        ufa_free);
		write_config(dirs, error);
		ufa_goto_iferror(error, freeres);
	}

	ret = true;
freeres:
	ufa_free(normdir);
	ufa_list_free(dirs);
	pthread_mutex_unlock(&global_mutex);
end:
	return ret;
}

bool ufa_config_remove_dir(const char *dir, struct ufa_error **error)
{
	ufa_debug("Removing dir '%s'", dir);

	char *dirs_file       = NULL;
	bool ret              = false;
	struct ufa_list *dirs = NULL;

	ufa_goto_iferror(error, end);

	pthread_mutex_lock(&global_mutex);

	dirs = ufa_config_dirs(true, error);
	ufa_goto_iferror(error, freeres);

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
	ufa_goto_iferror(error, freeres);

	ret = true;
freeres:
	pthread_mutex_unlock(&global_mutex);
	ufa_free(dirs_file);
end:
	return ret;
}

char *ufa_config_getlogfilepath(struct ufa_error **error)
{
	if (!check_and_create_config_dir(error)) {
		goto end;
	}

	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	char *logfile = ufa_util_joinpath(cfg_dir, LOG_FILE_NAME, NULL);

	ufa_free(cfg_dir);

	return logfile;

end:
	return NULL;
}


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */


static char *get_config_dirs_filepath()
{
	char *cfg_dir   = ufa_util_config_dir(CONFIG_DIR_NAME);
	char *dirs_file = ufa_util_joinpath(cfg_dir, DIRS_FILE_NAME, NULL);
	ufa_free(cfg_dir);
	return dirs_file;
}

static bool check_and_create_config_dir(struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	bool ret           = false;
	char *dirs_file    = NULL;
	char *cfg_dir      = NULL;
	FILE *out          = NULL;
	char *base_cfg_dir = NULL;

	cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);

	if (!ufa_util_isdir(cfg_dir)) {
		base_cfg_dir = ufa_util_config_dir(NULL);
		if (ufa_util_isdir(base_cfg_dir)) {
			ufa_debug("Creating dir '%s'", cfg_dir);
			bool created = ufa_util_mkdir(cfg_dir, NULL);
			if (!created) {
				ufa_error_new(error,
					      UFA_ERROR_FILE,
					      "Could not create dir: %s",
					      cfg_dir);
				goto freeres;
			}
		} else {
			ufa_error_new(error,
				      UFA_ERROR_NOTDIR,
				      "Base config dir does not exist: %s",
				      base_cfg_dir);
			goto freeres;
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
			goto freeres;
		}
		fprintf(out, DIRS_FILE_DEFAULT_STRING);
	}

	ret = true;

freeres:
	if (out) {
		fclose(out);
	}
	ufa_free(dirs_file);
	ufa_free(cfg_dir);
	ufa_free(base_cfg_dir);
end:
	return ret;
}

static void write_config(const struct ufa_list *list, struct ufa_error **error)
{

	ufa_debug("Saving config gile");

	ufa_return_if(list == NULL);
	ufa_return_iferror(error);

	char *dirs_file = get_config_dirs_filepath();
	char *tmp_file  = NULL;
	char *cfg_dir   = NULL;
	FILE *file      = NULL;

	// Create temp file
	cfg_dir  = ufa_util_config_dir(CONFIG_DIR_NAME);
	tmp_file = ufa_util_joinpath(cfg_dir, "ufacfgXXXXXX", NULL);

	int fd = mkstemp(tmp_file);
	if (fd == -1) {
		perror("mkstemp");
		ufa_error_new(error, UFA_ERROR_FILE,
			"Could not create temp file for writing: %s",
			tmp_file);
		goto freeres;
	}

	file = fdopen(fd, "w");

	if (file == NULL) {
		ufa_error_new(error, UFA_ERROR_FILE,
			"Could not open temp file for writing: %s",
			tmp_file);
		goto freeres;
	}

	// Write header
	fprintf(file, DIRS_FILE_DEFAULT_STRING);

	if (ufa_log_is_logging(UFA_LOG_DEBUG)) {
		ufa_debug("Writing %d dirs to '%s'", ufa_list_size(list),
			tmp_file);
	}

	// Write all dirs
	for (UFA_LIST_EACH(i, list)) {
		char *dir = (char *) i->data;
		fprintf(file, "%s\n", dir);
	}

	ufa_debug("%d lines written in config file", ufa_list_size(list));

	fflush(file);
	fsync(fileno(file));
	fclose(file);
	file = NULL;

	ufa_debug("Renamig '%s' to '%s'", tmp_file, dirs_file);
	// Replace file. Atomic operation
	if (rename(tmp_file, dirs_file) != 0) {
		ufa_error_new(error, UFA_ERROR_FILE,
			"Could not rename temp file '%s' to '%s'",
			tmp_file, dirs_file);
		goto freeres;
	}

freeres:
	if (file) {
	    	fclose(file); // em caso de erro
	}
	ufa_free(dirs_file);
	ufa_free(tmp_file);
	ufa_free(cfg_dir);
}
