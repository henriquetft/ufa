#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "util/logging.h"

static int loglevel = UFA_LOG_DEBUG;

#define IS_VALID_LOG_LEVEL(level) (level >= UFA_LOG_DEBUG && level <= UFA_LOG_FATAL)

struct log_level_attrs_s {
	const char *prefix_str;
	const char *color;
};

static const struct log_level_attrs_s log_level_attr[5] = {
    { "[DEBUG]:   ", "\033[0;34m", },
    { "[INFO ]:   ", "\033[0;36m", },
    { "[WARN ]:   ", "\033[0;33m", },
    { "[ERROR]:   ", "\033[0;31m", },
    { "[FATAL]:   ", "\033[0;31m", },
};


char *
ufa_log_level_to_str(ufa_log_level_t level)
{
    if (!IS_VALID_LOG_LEVEL(level)) {
        return NULL;
    }
    return log_level_attr[level].prefix_str;
}


static void
_ufa_log(ufa_log_level_t level, char *format, va_list ap)
{
    static int is_a_tty = -1;
    if (is_a_tty == -1) {
        is_a_tty = isatty(STDOUT_FILENO);
    }

    if (!IS_VALID_LOG_LEVEL(level)) {
        return;
    }

    /* man 3 stdarg */
    FILE *file = stdout;
	if (is_a_tty) {
        fprintf(stdout, log_level_attr[level].color);
    }
    fprintf(file, "%s", ufa_log_level_to_str(level));
    vfprintf(file, format, ap);
    fprintf(file, "\n");

	if (is_a_tty) {
        printf("\033[0;0m");
    }

    fflush(file);
}

void
ufa_debug(char *format, ...)
{
    if (loglevel <= UFA_LOG_DEBUG) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(UFA_LOG_DEBUG, format, ap);
        va_end(ap);
    }
}

void
ufa_info(char *format, ...)
{
    if (loglevel <= UFA_LOG_INFO) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(UFA_LOG_INFO, format, ap);
        va_end(ap);
    }
}

void
ufa_warn(char *format, ...)
{
    if (loglevel <= UFA_LOG_WARN) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(UFA_LOG_WARN, format, ap);
        va_end(ap);
    }
}

void
ufa_error(char *format, ...)
{
    if (loglevel <= UFA_LOG_ERROR) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(UFA_LOG_ERROR, format, ap);
        va_end(ap);
    }
}

void
ufa_fatal(char *format, ...)
{
    if (loglevel <= UFA_LOG_FATAL) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(UFA_LOG_FATAL, format, ap);
        va_end(ap);
    }
}
