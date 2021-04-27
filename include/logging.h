#ifndef LOGGING_H_
#define LOGGING_H_

#include <stdio.h>
#include <string.h>

typedef enum ufa_log_level_e {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4,

} ufa_log_level_t;

void
ufa_debug(char *format, ...);

void
ufa_info(char *format, ...);

void
ufa_warn(char *format, ...);

void
ufa_error(char *format, ...);

void
ufa_fatal(char *format, ...);

#endif /* LOGGING_H_ */
