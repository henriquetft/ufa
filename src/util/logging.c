#include <stdarg.h>
#include <stdio.h>
#include "util/logging.h"

static int loglevel = LOG_OFF;

char *
ufa_log_level_to_str(ufa_log_level_t level)
{
    switch (level) {
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARN:
        return "WARN";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    default:
        return NULL;
    }
}

ufa_log_level_t
ufa_log_str_to_level(char *level)
{
    if (!strcasecmp(level, "DEBUG")) {
        return DEBUG;
    } else if (!strcasecmp(level, "INFO")) {
        return INFO;
    } else if (!strcasecmp(level, "WARN")) {
        return WARN;
    } else if (!strcasecmp(level, "ERROR")) {
        return ERROR;
    } else if (!strcasecmp(level, "FATAL")) {
        return FATAL;
    } else {
        return -1;
    }
}

static void
_ufa_log(ufa_log_level_t level, char *format, va_list ap)
{
    if (loglevel > level)
        return;

    /* man 3 stdarg */

    FILE *file = stdout;

    fprintf(file, "[LOG %s] ", ufa_log_level_to_str(level));
    vfprintf(file, format, ap);
    fprintf(file, "\n");

    fflush(file);
}

void
ufa_debug(char *format, ...)
{
    if (loglevel <= DEBUG) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(DEBUG, format, ap);
        va_end(ap);
    }
}

void
ufa_info(char *format, ...)
{
    if (loglevel <= INFO) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(INFO, format, ap);
        va_end(ap);
    }
}

void
ufa_warn(char *format, ...)
{
    if (loglevel <= WARN) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(WARN, format, ap);
        va_end(ap);
    }
}

void
ufa_error(char *format, ...)
{
    if (loglevel <= ERROR) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(ERROR, format, ap);
        va_end(ap);
    }
}

void
ufa_fatal(char *format, ...)
{
    if (loglevel <= FATAL) {
        va_list ap;
        va_start(ap, format);
        _ufa_log(FATAL, format, ap);
        va_end(ap);
    }
}
