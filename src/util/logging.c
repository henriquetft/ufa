/* ========================================================================== */
/* Copyright (c) 2021 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Logging functions (implementation of logging.h)                            */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <syslog.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static FILE *global_file = NULL;

static int loglevel = UFA_LOG_OFF;
static bool log_to_syslog = false;

#define IS_VALID_LOG_LEVEL(level) (level >= UFA_LOG_DEBUG \
				   && level <= UFA_LOG_FATAL)

struct log_level_attrs {
	const char *prefix_str;
	const char *color;
	const int syslog_priority;
};


static const struct log_level_attrs log_level_attr[5] = {
    {
	"[DEBUG]: ",
	"\033[0;34m",
	LOG_DEBUG,
    },
    {
	"[INFO ]: ",
	"\033[0;36m",
	LOG_INFO,
    },
    {
	"[WARN ]: ",
	"\033[0;33m",
	LOG_WARNING,
    },
    {
	"[ERROR]: ",
	"\033[0;31m",
	LOG_ERR,
    },
    {
	"[FATAL]: ",
	"\033[0;31m",
	LOG_CRIT,
    },
};


/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void log_syslog(enum ufa_log_level level,
		       const char *format, va_list ap);
static void log_file(enum ufa_log_level level, const char *format, va_list ap);
static void write_log(enum ufa_log_level level, const char *format, va_list ap);

/* ========================================================================== */
/* FUNCTIONS FROM logging.h                                                   */
/* ========================================================================== */

void ufa_log_setlevel(enum ufa_log_level level)
{
	loglevel = level;
}

enum ufa_log_level ufa_log_getlevel()
{
	return loglevel;
}

const char *ufa_log_level_to_str(enum ufa_log_level level)
{
	if (!IS_VALID_LOG_LEVEL(level)) {
		return NULL;
	}
	return log_level_attr[level].prefix_str;
}


void ufa_log_use_syslog()
{
	openlog("UFA", LOG_PID | LOG_CONS, LOG_USER);
	log_to_syslog = true;
}

void ufa_log_use_file(FILE *file_log)
{
	global_file = file_log;
	log_to_syslog = false;
}


static void log_syslog(enum ufa_log_level level, const char *format, va_list ap)
{
	vsyslog(log_level_attr[level].syslog_priority, format, ap);
}



void ufa_debug(const char *format, ...)
{
	if (loglevel <= UFA_LOG_DEBUG) {
		va_list ap;
		va_start(ap, format);
		write_log(UFA_LOG_DEBUG, format, ap);
		va_end(ap);
	}
}

void ufa_info(const char *format, ...)
{
	if (loglevel <= UFA_LOG_INFO) {
		va_list ap;
		va_start(ap, format);
		write_log(UFA_LOG_INFO, format, ap);
		va_end(ap);
	}
}

void ufa_warn(const char *format, ...)
{
	if (loglevel <= UFA_LOG_WARN) {
		va_list ap;
		va_start(ap, format);
		write_log(UFA_LOG_WARN, format, ap);
		va_end(ap);
	}
}

void ufa_error(const char *format, ...)
{
	if (loglevel <= UFA_LOG_ERROR) {
		va_list ap;
		va_start(ap, format);
		write_log(UFA_LOG_ERROR, format, ap);
		va_end(ap);
	}
}

void ufa_fatal(const char *format, ...)
{
	if (loglevel <= UFA_LOG_FATAL) {
		va_list ap;
		va_start(ap, format);
		write_log(UFA_LOG_FATAL, format, ap);
		va_end(ap);
	}
}

bool ufa_log_is_logging(enum ufa_log_level level)
{
	return loglevel <= level;
}

enum ufa_log_level ufa_log_level_from_str(const char *level)
{
	if (!strcasecmp("off", level)) {
		return UFA_LOG_OFF;
	} else if (!strcasecmp("debug", level)) {
		return UFA_LOG_DEBUG;
	} else if (!strcasecmp("info", level)) {
		return UFA_LOG_INFO;
	} else if (!strcasecmp("warn", level)) {
		return UFA_LOG_WARN;
	} else if (!strcasecmp("error", level)) {
		return UFA_LOG_ERROR;
	} else if (!strcasecmp("fatal", level)) {
		return UFA_LOG_FATAL;
	}
	return UFA_LOG_OFF;
}

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */


static void write_log(enum ufa_log_level level, const char *format, va_list ap)
{
	if (!IS_VALID_LOG_LEVEL(level)) {
		return;
	}

	if (log_to_syslog) {
		log_syslog(level, format, ap);
	} else {
		log_file(level, format, ap);
	}
}

static void log_file(enum ufa_log_level level, const char *format, va_list ap)
{
	if (global_file == NULL) {
		global_file = stdout;
	}

	FILE *file = global_file;

	static int is_a_tty = -1;
	if (is_a_tty == -1) {
		is_a_tty = isatty(fileno(file));
	}

	/* man 3 stdarg */
	if (is_a_tty) {
		fprintf(file, "%s", log_level_attr[level].color);
	}
	fprintf(file, "%s", ufa_log_level_to_str(level));
	vfprintf(file, format, ap);
	fprintf(file, "\n");

	if (is_a_tty) {
		fprintf(file, "\033[0;0m");
	}

	fflush(file);
}
