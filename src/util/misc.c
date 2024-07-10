/* ========================================================================== */
/* Copyright (c) 2021-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Miscellaneous utility functions (implementation of misc.h).                */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/string.h"
#include "util/misc.h"
#include "util/error.h"
#include "util/list.h"
#include <errno.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>


/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static char *join_path(const char *delim, const char *first_element,
		       va_list *ap);


/* ========================================================================== */
/* FUNCTIONS FROM misc.h                                                      */
/* ========================================================================== */


char *ufa_util_joinpath(const char *first_element, ...)
{
	va_list ap;
	va_start(ap, first_element);
	char *str = join_path(UFA_FILE_SEPARATOR, first_element, &ap);
	va_end(ap);

	return str;
}
/**
 * Returns the file name of a file path.
 * It returns a newly allocated string with the last part of the path
 * (separated by file separador).
 */
char *ufa_util_getfilename(const char *filepath)
{
	// FIXME rewrite this function
	struct ufa_list *split = ufa_str_split(filepath, "/"); // FIXME
	struct ufa_list *last = ufa_list_get_last(split);
	char *last_part = ufa_str_dup((char *) last->data);
	ufa_list_free(split);
	return last_part;
}

char *ufa_util_dirname(const char *filepath)
{
	char *ret = NULL;
	char *s = ufa_str_dup(filepath);
	ret = ufa_str_dup(dirname(s));
	ufa_free(s);
	return ret;
}

char *ufa_util_abspath(const char *path)
{
	char buf[PATH_MAX];
	if (realpath(path, buf) == NULL) {
		return NULL;
	}

	return ufa_str_dup(buf);
}

void *ufa_util_abspath2(const char *path, char *dest_buf)
{
	realpath(path, dest_buf);
}



int ufa_util_isdir(const char *filename)
{
	struct stat st;
	return (stat(filename, &st) == 0 && S_ISDIR(st.st_mode));
}

int ufa_util_isfile(const char *filename)
{
	struct stat st;
	return (stat(filename, &st) == 0 && S_ISREG(st.st_mode));
}

char *ufa_util_get_current_dir()
{
	char cwd[PATH_MAX];
	if (getcwd(cwd, PATH_MAX-1) != NULL) {
		return ufa_str_dup(cwd);
	} else {
		perror("getcwd() error");
		return NULL;
	}
}

char *ufa_util_get_home_dir()
{
	struct passwd pwd;
	struct passwd *result;
	size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
	char *buf = malloc(bufsize);

	if ((getpwuid_r(getuid(), &pwd, buf, bufsize, &result)) != 0) {
		perror("getpwuid_r() error.");
		return NULL;
	}

	char *ret = ufa_str_dup(pwd.pw_dir);
	ufa_free(buf);
	return ret;
}

char *ufa_util_config_dir(const char *appname)
{
	char *config_dir = NULL;
	const char *s = getenv("XDG_CONFIG_HOME");
	if (s == NULL) {
		char *homedir = ufa_util_get_home_dir();
		config_dir = ufa_util_joinpath(homedir, ".config", appname, NULL);
		ufa_free(homedir);
	} else {
		config_dir = ufa_util_joinpath(s, appname, NULL);
	}
	return config_dir;
}

bool ufa_util_mkdir(const char *dir, struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	mode_t mode = 0777;
	if (mkdir(dir, mode) == 0) {
		return true;
	}

	ufa_error_new(error, errno, strerror(errno));
	return false;
}

bool ufa_util_rmdir(const char *dir, struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	if (rmdir(dir) == 0) {
		return true;
	}

	ufa_error_new(error, errno, strerror(errno));
	return false;
}

bool ufa_util_remove_file(const char *filepath, struct ufa_error **error)
{
	ufa_return_val_iferror(error, false);

	if (remove(filepath) == 0) {
		return true;
	}

	ufa_error_new(error, errno, strerror(errno));
	return false;
}


inline void *ufa_malloc(size_t size)
{
	return malloc(size);
}

void *ufa_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

void *ufa_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

inline void ufa_free(void *p)
{
	free(p);
}

bool *ufa_bool_dup(bool i)
{
	bool *n = ufa_malloc(sizeof(bool));
	*n = i;
	return n;
}

double *ufa_double_dup(double number)
{
	double *n = ufa_malloc(sizeof(double));
	*n = number;
	return n;
}

long *ufa_long_dup(long number)
{
	long *n = ufa_malloc(sizeof(long));
	*n = number;
	return n;
}

int *ufa_intptr_dup(int *i)
{
	if (i == NULL) {
		return NULL;
	}
	int *n = ufa_malloc(sizeof(int));
	*n = *i;
	return n;
}

int *ufa_int_dup(int i)
{
	int *n = ufa_malloc(sizeof(int));
	*n = i;
	return n;
}


char *ufa_util_resolvepath(const char *filename)
{
	const size_t MAX_SIZE = PATH_MAX;

	char *buf;
	char *new = buf = ufa_malloc(MAX_SIZE * sizeof(char));
	new[0] = '\0';

	char *path = filename;

	if (*path != '/') {
		char *s = ufa_util_get_current_dir();
		strncat(new, s, MAX_SIZE-1);
	}

	bool stop = false;
	//strlen(new) + count_chars >= MAX_SIZE-1
	while (!stop && *path != '\0') {

		if (*path == '/') {
			path++;
			continue;
		}

		if (*path == '.') {
			if (path[1] == '\0' || path[1] == '/') {
				path++;
				continue;
			}
			// ".."
			if (path[1] == '.') {
				if (path[2] == '\0' || path[2] == '/') {
					path += 2;
					if (strlen(new) == 0) {
						continue;
					}

					char *n = new + strlen(new);
					while (*(--n) != '/');
					*n = '\0';
					continue;
				}
			}
		}

		int count_chars = 0;
		char *n = new + strlen(new);
		while (*(path+count_chars) != '\0' && *(path+count_chars) != '/') {
			stop = strlen(new) + count_chars >= MAX_SIZE-1;
			if (stop) {
				break;
			}
			count_chars++;
		}

		if (!stop) {
			n[0] = '/';
			memcpy(n + 1, path, count_chars);
			n[count_chars + 1] = '\0';
			path += count_chars;
		}
	}
	return buf;
}


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static char *join_path(const char *delim, const char *first_element,
		       va_list *ap)
{
	char *next = (char *)first_element;
	char *buf = ufa_str_dup(next);
	int ends_with_delim = ufa_str_endswith(buf, delim);

	while ((next = va_arg(*ap, char*)) != NULL) {

		if (!ends_with_delim && !ufa_str_startswith(next, delim)) {
			char *temp = ufa_str_concat(buf, delim);
			ufa_free(buf);
			buf = temp;

		} else if (ends_with_delim && ufa_str_startswith(next, delim)) {
			next = next + strlen(delim);
		}

		char *temp = ufa_str_concat(buf, next);
		ufa_free(buf);
		buf = temp;

		ends_with_delim = ufa_str_endswith(buf, delim);
	}
	return buf;
}

