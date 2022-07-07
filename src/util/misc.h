#ifndef MISC_H_
#define MISC_H_

#include "list.h"
#include <stdarg.h>

#define UFA_FILE_SEPARATOR "/"

/** Size of an array */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/** Macro to use with FOR statement to iterate over arrays
 * e.g. for (UFA_ARRAY_EACH(i, array)) { ... }*/
#define UFA_ARRAY_EACH(i, arr) size_t i = 0; i < ARRAY_SIZE(arr); i++

char *ufa_util_joinpath(int args, const char *first_element, ...);

/**
 * Returns the file name of a file path.
 * It returns a newly allocated string with the last part of the path
 * (separated by file separador).
 */
char *ufa_util_getfilename(const char *filepath);

char *ufa_util_joinstr(char *delim, int args, const char *first_element, ...);

char *ufa_util_strcat(const char *str1, const char *str2);

int ufa_util_strequals(const char *str1, const char *str2);

int ufa_util_isdir(const char *filename);

int ufa_util_isfile(const char *filename);

char *ufa_util_get_current_dir();

struct ufa_list *ufa_str_split(const char *str, const char *delim);

int ufa_str_startswith(const char *str, const char *prefix);

int ufa_str_endswith(const char *str, const char *suffix);

char *ufa_strdup(const char *str);

char *ufa_str_multiply(const char *str, int times);

int ufa_str_count(const char *str, const char *part);

char *ufa_str_vprintf(char const *format, va_list ap);

char *ufa_str_sprintf(char const *format, ...);

void ufa_str_replace(char *str, char old, char new);

#endif /* MISC_H_ */
