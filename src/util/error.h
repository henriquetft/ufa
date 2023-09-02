/* ========================================================================== */
/* Copyright (c) 2021 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for error handling utilities.                                  */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_ERROR_H_
#define UFA_ERROR_H_

#include <stdarg.h>

struct ufa_error {
	int code;
	char *message;
};

/** Macro to check errors. Arg expected: pointer to pointer to ufa_error */
#define HAS_ERROR(error) (error && *error)

void ufa_error_new(struct ufa_error **error, int code, char *format, ...);
void ufa_error_free(struct ufa_error *error);
void ufa_error_abort(const struct ufa_error *error);
void ufa_error_exit(const struct ufa_error *error, int status);
void ufa_error_print_prefix(const struct ufa_error *error, const char *prefix);
void ufa_error_print(const struct ufa_error *error);
void ufa_error_print_and_free(struct ufa_error *error);
void ufa_error_print_and_free_prefix(struct ufa_error *error,
				     const char *prefix);
struct ufa_error *ufa_error_clone(const struct ufa_error *error);

#endif /* UFA_ERROR_H_ */