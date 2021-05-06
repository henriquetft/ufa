#ifndef MISC_H_
#define MISC_H_

#include <stdarg.h>
#include "list.h"


char *
ufa_util_join_path(int args, const char *first_element, ...);

char *
ufa_util_join_str(char *delim, int args, const char *first_element, ...);

char *
ufa_util_strcat(const char *str1, const char *str2);

int
ufa_util_strequals(const char *str1, const char *str2);

int
ufa_util_isdir(const char *filename);

int
ufa_util_isfile(const char *filename);

char *
ufa_util_get_current_dir();

ufa_list_t *
ufa_util_str_split(const char *str, const char *delim);

int
ufa_util_str_startswith(const char *str, const char *prefix);

int
ufa_util_str_endswith(const char *str, const char *suffix);

char *
ufa_strdup(const char *str);

char *
ufa_util_str_multiply(const char *str, int times);


#define UFA_FILE_SEPARATOR "/"

int
ufa_util_strcount(const char *str, const char *part);

char *
ufa_str_vprintf(char const *format, va_list ap);

char *
ufa_str_sprintf(char const *format, ...);


#endif /* MISC_H_ */
