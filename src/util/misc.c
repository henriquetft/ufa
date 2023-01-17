/* ========================================================================== */
/* Copyright (c) 2021 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Miscellaneous utility functions (implementation of misc.h).                */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#include "misc.h"
#include "error.h"
#include "list.h"
#include <ctype.h>
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


int ufa_str_startswith(const char *str, const char *prefix)
{
	int x = (strstr(str, prefix) == str);
	return x;
}

int ufa_str_endswith(const char *str, const char *suffix)
{
	int p = strlen(str) - strlen(suffix);
	if (p < 0) {
		return 0;
	}
	char *pos = ((char *)str) + p;
	int r = strstr(pos, suffix) == pos;
	return r;
}

static char *_join_path(const char *delim,
			const char *first_element,
			va_list *ap)
{
	char *next = (char *)first_element;
	char *buf = ufa_strdup(next);
	int ends_with_delim = ufa_str_endswith(buf, delim);

	while ((next = va_arg(*ap, char *)) != NULL) {

		if (!ends_with_delim && !ufa_str_startswith(next, delim)) {
			char *temp = ufa_util_strcat(buf, delim);
			free(buf);
			buf = temp;

		} else if (ends_with_delim && ufa_str_startswith(next, delim)) {
			next = next + strlen(delim);
		}

		char *temp = ufa_util_strcat(buf, next);
		free(buf);
		buf = temp;

		ends_with_delim = ufa_str_endswith(buf, delim);
	}
	return buf;
}

char *ufa_util_joinpath(const char *first_element, ...)
{
	va_list ap;
	va_start(ap, first_element);
	char *str = _join_path(UFA_FILE_SEPARATOR, first_element, &ap);
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
	char *last_part = ufa_strdup((char *)last->data);
	ufa_list_free_full(split, free);
	return last_part;
}

char *ufa_util_dirname(const char *filepath)
{
	char *ret = NULL;
	char *s = ufa_strdup(filepath);
	ret = ufa_strdup(dirname(s));
	ufa_free(s);
	return ret;
}

char *ufa_util_abspath(const char *path)
{
	char buf[PATH_MAX];
	if (realpath(path, buf) == NULL) {
		return NULL;
	}

	return ufa_strdup(buf);
}


struct ufa_list *ufa_str_split(const char *str, const char *delim)
{
	struct ufa_list *list = NULL;

	char *s = ufa_strdup(str);
	char *ptr = strtok(s, delim);

	while (ptr != NULL) {
		list = ufa_list_append(list, ufa_strdup(ptr));
		ptr = strtok(NULL, delim);
	}

	free(s);
	return list;
}

char *ufa_util_strcat(const char *str1, const char *str2)
{
	int len = (strlen(str1) + strlen(str2) + 1);
	char *new_str = malloc(len * sizeof *new_str);

	if (new_str != NULL) {
		snprintf(new_str, len, "%s%s", str1, str2);
	}
	return new_str;
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
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		return ufa_strdup(cwd);
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

	char *ret = ufa_strdup(pwd.pw_dir);
	free(buf);
	return ret;
}

char *ufa_util_config_dir(const char *appname)
{
	char *config_dir = NULL;
	const char *s = getenv("XDG_CONFIG_HOME");
	if (s == NULL) {
		char *homedir = ufa_util_get_home_dir();
		config_dir = ufa_util_joinpath(homedir, ".config", appname, NULL);
		free(homedir);
	} else {
		config_dir = ufa_util_joinpath(s, appname, NULL);
	}
	return config_dir;
}

bool ufa_util_mkdir(const char *dir, struct ufa_error **error)
{
	mode_t mode = 0777;
	if (mkdir(dir, mode) == 0) {
		return true;
	}

	ufa_error_new(error, errno, strerror(errno));
	return false;
}

bool ufa_util_rmdir(const char *dir, struct ufa_error **error)
{
	if (rmdir(dir) == 0) {
		return true;
	}

	ufa_error_new(error, errno, strerror(errno));
	return false;
}

bool ufa_util_remove_file(const char *filepath, struct ufa_error **error)
{
	if (remove(filepath) == 0) {
		return true;
	}

	ufa_error_new(error, errno, strerror(errno));
	return false;
}

int ufa_str_equals(const char *str1, const char *str2)
{
	/* TODO write without strcmp */
	return !strcmp(str1, str2);
}

char *ufa_strdup(const char *str)
{
	return strdup(str);
}

char *ufa_str_multiply(const char *str, int times)
{
	// if (times < 0) {
	//     return NULL;
	// }
	int len = strlen(str);
	int total = times * len * sizeof(char) + 1;
	char *new_str = (char *)malloc(total);

	strcpy(new_str, "");
	for (int x = 0; x < times; x++) {
		strcat(new_str + len * x, str);
	}
	return new_str;
}

int ufa_str_count(const char *str, const char *part)
{
	int len_part = strlen(part);
	int found = 0;
	char *f = str;
	while ((f = strstr(f, part)) != NULL) {
		found++;
		f = f + len_part;
	}
	return found;
}

char *ufa_str_vprintf(char const *format, va_list ap)
{
	va_list args2;
	va_copy(args2, ap);
	int len = vsnprintf(NULL, 0, format, ap);
	char *buffer = (char *)calloc(len + 1, 1);
	vsprintf(buffer, format, args2);
	return buffer;
}

char *ufa_str_sprintf(char const *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char *result = ufa_str_vprintf(format, ap);
	va_end(ap);
	return result;
}

void ufa_str_replace(char *str, char old, char new)
{
	char *p = str;
	while ((p = strchr(p, old))) {
		*p = new;
		p++;
	}
}

static int istrimchar(char c)
{
	return (isblank(c) || c == '\r' || c == '\n');
}

char *ufa_str_ltrim(char *s)
{
	char *tmp = s;
	while (istrimchar(*tmp)) {
		tmp++;
	}
	memmove(s, tmp, strlen(tmp)+1);

	return s;
}

char *ufa_str_rtrim(char *s)
{
	int last = strlen(s);
	char *tmp = s + last - 1;
	while (tmp >= s) {
		if (istrimchar(*tmp)) {
			*tmp = '\0';
		} else {
			break;
		}
		tmp--;
	}
	return s;
}

char *ufa_str_trim(char *s)
{
	s = ufa_str_rtrim(s);
	s = ufa_str_ltrim(s);
	return s;
}

int ufa_str_hash(const char *str)
{
	int h = 0;
	while (*str++ != '\0') {
		h = h*31 + *str;
	}
	return h;
}

void ufa_free(void *p)
{
	free(p);
}