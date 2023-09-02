/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* String utility functions (implementation of string.h).                     */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/string.h"
#include "util/misc.h"
#include "util/list.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define IS_TRIM_CHAR(c)  (isblank((c)) || (c) == '\r' || (c) == '\n')


/* ========================================================================== */
/* FUNCTIONS FROM string.h                                                    */
/* ========================================================================== */

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


struct ufa_list *ufa_str_split(const char *str, const char *delim)
{
	struct ufa_list *list = NULL;

	char *s = ufa_str_dup(str);
	char *ptr = strtok(s, delim);

	while (ptr != NULL) {
		list = ufa_list_prepend2(list, ufa_str_dup(ptr), ufa_free);
		ptr = strtok(NULL, delim);
	}

	ufa_free(s);
	return ufa_list_reverse(list);
}

char *ufa_str_concat(const char *str1, const char *str2)
{
	size_t len = (strlen(str1) + strlen(str2) + 1);
	char *new_str = ufa_malloc(len * sizeof *new_str);

	if (new_str != NULL) {
		snprintf(new_str, len, "%s%s", str1, str2);
	}
	return new_str;
}

bool ufa_str_equals(const char *str1, const char *str2)
{
	return !strcmp(str1, str2);
}

char *ufa_str_dup(const char *str)
{
	return strdup(str);
}

char *ufa_str_multiply(const char *str, int times)
{
	// if (times < 0) {
	//     return NULL;
	// }
	size_t len = strlen(str);
	int total = times * len * sizeof(char) + 1;
	char *new_str = (char *) ufa_malloc(total);

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
	char *f = (char *) str;
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
	char *buffer = (char *) ufa_calloc(len + 1, 1);
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

char *ufa_str_ltrim(char *s)
{
	char *tmp = s;
	while (IS_TRIM_CHAR(*tmp)) {
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
		if (IS_TRIM_CHAR(*tmp)) {
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

bool ufa_str_to_double(const char *str, double *number)
{
	int ret = sscanf(str, "%lf", number);
	return ret > 0;
}

bool ufa_str_to_long(const char *str, long *number)
{
	int ret = sscanf(str, "%ld", number);
	return ret > 0;
}

char *ufa_str_join_list(struct ufa_list *list, const char *delim,
			const char *left, const char *right)
{
	if (list == NULL) {
		return ufa_str_dup("");
	}

	int char_capacity = 20;
	int total         = 0;
	delim  =  (delim == NULL) ? "" : delim;
	left   =  (left == NULL)  ? "" : left;
	right  =  (right == NULL) ? "" : right;

	char *buffer = ufa_calloc(sizeof(char), char_capacity + 1);
	buffer[0] = '\0';

	for (UFA_LIST_EACH(i, list)) {
		char *value = (char *) i->data;
		char *n = ufa_str_sprintf("%s%s%s%s",
					  left,
					  value,
					  right,
					  delim);
		size_t length = strlen(n);

		total += length;
		if (total > char_capacity) {
			buffer = ufa_realloc(buffer, total + 1);
			char_capacity = total;
		}
		strcat(buffer, n);
		ufa_free(n);
	}
	size_t len_delim = strlen(delim);

	if (len_delim > 0 && strlen(buffer) - len_delim > 0) {
		buffer[strlen(buffer) - len_delim] = '\0';
	}
	return buffer;
}