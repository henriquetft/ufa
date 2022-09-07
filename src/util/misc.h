#ifndef MISC_H_
#define MISC_H_

#include "list.h"
#include "error.h"
#include <stdarg.h>
#include <stdbool.h>


#define UFA_FILE_SEPARATOR "/"

/** Size of an array */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ufa_return_val_ifnot(expr, val) if (!(expr)) { return val; }

#define if_goto(cond, label)    if (cond) { goto label; }

/** Macro to use with FOR statement to iterate over arrays
 * e.g. for (UFA_ARRAY_EACH(i, array)) { ... }*/
#define UFA_ARRAY_EACH(i, arr) size_t i = 0; i < ARRAY_SIZE(arr); i++

char *ufa_util_joinpath(const char *first_element, ...);

/**
 * Returns the file name of a file path.
 * It returns a newly allocated string with the last part of the path
 * (separated by file separador).
 */
char *ufa_util_getfilename(const char *filepath);

char *ufa_util_dirname(const char *filepath);

char *ufa_util_abspath(const char *path);

char *ufa_util_strcat(const char *str1, const char *str2);

int ufa_util_strequals(const char *str1, const char *str2);

int ufa_util_isdir(const char *filename);

int ufa_util_isfile(const char *filename);

char *ufa_util_get_current_dir();

char *ufa_util_get_home_dir();

char *ufa_util_config_dir(const char *appname);

bool ufa_util_mkdir(const char *dir, struct ufa_error **error);

struct ufa_list *ufa_str_split(const char *str, const char *delim);

int ufa_str_startswith(const char *str, const char *prefix);

int ufa_str_endswith(const char *str, const char *suffix);

char *ufa_strdup(const char *str);

char *ufa_str_multiply(const char *str, int times);

int ufa_str_count(const char *str, const char *part);

char *ufa_str_vprintf(char const *format, va_list ap);

char *ufa_str_sprintf(char const *format, ...);

void ufa_str_replace(char *str, char old, char new);

char *ufa_str_ltrim(char *s);

char *ufa_str_rtrim(char *s);

char *ufa_str_trim(char *s);

int ufa_str_hash(const char *str);

void ufa_free(void *p);

#endif /* MISC_H_ */
