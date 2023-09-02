/* ========================================================================== */
/* Copyright (c) 2021 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Error handling utilities (implementation of error.h)                       */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/error.h"
#include "util/misc.h"
#include "util/string.h"
#include <stdio.h>
#include <stdlib.h>


void ufa_error_new(struct ufa_error **error, int code, char *format, ...)
{
	if (error == NULL) {
		return;
	}
	va_list ap;
	va_start(ap, format);
	char *m = ufa_str_vprintf(format, ap);
	va_end(ap);
	struct ufa_error *err = ufa_malloc(sizeof *err);
	err->code = code;
	err->message = m;
	*error = err;
}

void ufa_error_free(struct ufa_error *error)
{
	if (error != NULL) {
		ufa_free(error->message);
		ufa_free(error);
	}
}

void ufa_error_abort(const struct ufa_error *error)
{
	if (error != NULL) {
		fprintf(stderr, "error: %s\n", error->message);
		fflush(stderr);
		abort();
	}
}

void ufa_error_exit(const struct ufa_error *error, int status)
{
	if (error != NULL) {
		fprintf(stderr, "error: %s\n", error->message);
		fflush(stderr);
		exit(status);
	}
}

void ufa_error_print_prefix(const struct ufa_error *error, const char *prefix)
{
	if (error != NULL) {
		fprintf(stderr, "%s: %s\n", prefix, error->message);
	}
}

void ufa_error_print(const struct ufa_error *error)
{
	ufa_error_print_prefix(error, "error");
}

void ufa_error_print_and_free(struct ufa_error *error)
{
	ufa_error_print(error);
	ufa_error_free(error);
}

void ufa_error_print_and_free_prefix(struct ufa_error *error,
				     const char *prefix)
{
	ufa_error_print_prefix(error, prefix);
	ufa_error_free(error);
}

struct ufa_error *ufa_error_clone(const struct ufa_error *error)
{
	if (error == NULL) {
		return NULL;
	}
	struct ufa_error *err = ufa_malloc(sizeof *err);
	err->code = error->code;
	err->message = ufa_str_dup(error->message);

	return err;
}