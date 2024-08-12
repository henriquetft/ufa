/* ========================================================================== */
/* Copyright (c) 2021-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for Logging functions                                          */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef LOGGING_H_
#define LOGGING_H_

#include "util/error.h"
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>

enum ufa_log_level {
	UFA_LOG_DEBUG = 0,
	UFA_LOG_INFO  = 1,
	UFA_LOG_WARN  = 2,
	UFA_LOG_ERROR = 3,
	UFA_LOG_FATAL = 4,
	UFA_LOG_OFF   = INT_MAX,
};

void ufa_log_enablelogdetails(bool details);

void ufa_error_error(struct ufa_error *error);

enum ufa_log_level ufa_log_level_from_str(const char *level);

const char *ufa_log_level_to_str(enum ufa_log_level level);

void ufa_log_setlevel(enum ufa_log_level level);

enum ufa_log_level ufa_log_getlevel();

void ufa_log_use_syslog();

void ufa_log_use_file(FILE *file_log);

bool ufa_log_is_logging(enum ufa_log_level level);

void ufa_log_full(enum ufa_log_level loglevel, const char *sourcefile,
	          int line, const char *format, ...);

/**
 * Log message
 *
 * @param level Level (enum ufa_log_level)
 * @param format Format (str)
 */
#define ufa_log(level, format, ...) ufa_log_full(level, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ufa_debug(format, ...) ufa_log_full(UFA_LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ufa_info(format, ...) ufa_log_full(UFA_LOG_INFO, __FILE__, __LINE__,  format, ##__VA_ARGS__)
#define ufa_warn(format, ...) ufa_log_full(UFA_LOG_WARN, __FILE__, __LINE__,  format, ##__VA_ARGS__)
#define ufa_error(format, ...) ufa_log_full(UFA_LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ufa_fatal(format, ...) ufa_log_full(UFA_LOG_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#endif /* LOGGING_H_ */
