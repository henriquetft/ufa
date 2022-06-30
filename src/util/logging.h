#ifndef LOGGING_H_
#define LOGGING_H_

#include <limits.h>
#include <stdio.h>
#include <string.h>

enum ufa_log_level {
	UFA_LOG_DEBUG = 0,
	UFA_LOG_INFO = 1,
	UFA_LOG_WARN = 2,
	UFA_LOG_ERROR = 3,
	UFA_LOG_FATAL = 4,
	UFA_LOG_OFF = INT_MAX,
};

void ufa_debug(char *format, ...);

void ufa_info(char *format, ...);

void ufa_warn(char *format, ...);

void ufa_error(char *format, ...);

void ufa_fatal(char *format, ...);

enum ufa_log_level ufa_log_level_from_str(char *level);

void ufa_log_set(enum ufa_log_level level);

enum ufa_log_level ufa_log_get();

#endif /* LOGGING_H_ */
