/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for string utility function.                                   */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_STRING_H_
#define UFA_STRING_H_

#include "util/list.h"
#include "util/error.h"
#include <stdarg.h>
#include <stdbool.h>


/**
 * Returns empty string when x is NULL
 */
#define STR_NOTNULL(x) (((x) != NULL) ? (x) : "")


bool ufa_str_equals(const char *str1, const char *str2);

char *ufa_str_concat(const char *str1, const char *str2);

struct ufa_list *ufa_str_split(const char *str, const char *delim);

int ufa_str_startswith(const char *str, const char *prefix);

int ufa_str_endswith(const char *str, const char *suffix);

char *ufa_str_dup(const char *str);

char *ufa_str_multiply(const char *str, int times);

int ufa_str_count(const char *str, const char *part);

char *ufa_str_vprintf(char const *format, va_list ap);

char *ufa_str_sprintf(char const *format, ...);

void ufa_str_replace(char *str, char old, char new);

char *ufa_str_ltrim(char *s);

char *ufa_str_rtrim(char *s);

char *ufa_str_trim(char *s);

int ufa_str_hash(const char *str);

bool ufa_str_to_double(const char *str, double *number);

bool ufa_str_to_long(const char *str, long *number);


char *ufa_str_join_list(struct ufa_list *list, const char *delim,
			const char *left, const char *right);

#endif /* UFA_STRING_H_ */
