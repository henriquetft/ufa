/* ========================================================================== */
/* Copyright (c) 2021 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for Logging functions                                          */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

enum ufa_log_level {
	UFA_LOG_DEBUG = 0,
	UFA_LOG_INFO = 1,
	UFA_LOG_WARN = 2,
	UFA_LOG_ERROR = 3,
	UFA_LOG_FATAL = 4,
	UFA_LOG_OFF = INT_MAX,
};

void ufa_debug(const char *format, ...);

void ufa_info(const char *format, ...);

void ufa_warn(const char *format, ...);

void ufa_error(const char *format, ...);

void ufa_fatal(const char *format, ...);

enum ufa_log_level ufa_log_level_from_str(const char *level);
const char *ufa_log_level_to_str(enum ufa_log_level level);

void ufa_log_setlevel(enum ufa_log_level level);

enum ufa_log_level ufa_log_getlevel();

void ufa_log_use_syslog();

bool ufa_log_is_logging(enum ufa_log_level level);

#endif /* LOGGING_H_ */
