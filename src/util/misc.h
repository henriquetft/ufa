/* ========================================================================== */
/* Copyright (c) 2021-2023 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for miscellaneous utility function.                            */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_MISC_H_
#define UFA_MISC_H_

#include "list.h"
#include "error.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>


#define UFA_FILE_SEPARATOR "/"

/**
 * Size of an array
 */
#define   ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/**
 * Return val when expr is false
 */
#define   ufa_return_val_ifnot(expr, val) if (!(expr)) { return val; }

/**
 * Return val when expr is true
 */
#define   ufa_return_val_if(expr, val) if (expr) { return val; }

/**
 * Return (with no value) when expr is false
 */
#define   ufa_return_ifnot(expr) if (!(expr)) { return; }

/**
 * Return when expr ir true
 */
#define   ufa_return_if(expr) if (expr) { return; }

/**
 * Go to label if cond is true
 */
#define   if_goto(cond, label)   if (cond) { goto label; }

/**
 * Macro to use with FOR statement to iterate over arrays
 * e.g. for (UFA_ARRAY_EACH(i, array)) { ... }
 */
#define   UFA_ARRAY_EACH(i, arr) size_t i = 0; i < ARRAY_SIZE(arr); i++


char *ufa_util_joinpath(const char *first_element, ...);

/**
 * Returns the file name of a file path.
 * It returns a newly allocated string with the last part of the path
 * (separated by file separador).
 */
char *ufa_util_getfilename(const char *filepath);

char *ufa_util_dirname(const char *filepath);

char *ufa_util_abspath(const char *path);

void *ufa_util_abspath2(const char *path, char *dest_buf);


int ufa_util_isdir(const char *filename);

int ufa_util_isfile(const char *filename);

char *ufa_util_get_current_dir();

char *ufa_util_get_home_dir();

char *ufa_util_config_dir(const char *appname);

bool ufa_util_mkdir(const char *dir, struct ufa_error **error);

bool ufa_util_rmdir(const char *dir, struct ufa_error **error);

bool ufa_util_remove_file(const char *filepath, struct ufa_error **error);

void *ufa_malloc(size_t size);

void *ufa_calloc(size_t nmemb, size_t size);

void *ufa_realloc(void *ptr, size_t size);

void ufa_free(void *p);

bool *ufa_bool_dup(bool i);

double *ufa_double_dup(double number);

long *ufa_long_dup(long number);

int *ufa_intptr_dup(int *i);

int *ufa_int_dup(int i);

char *ufa_util_resolvepath(const char *dir);

#endif /* UFA_MISC_H_ */
