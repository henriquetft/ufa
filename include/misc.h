#ifndef MISC_H_
#define MISC_H_

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

#endif /* MISC_H_ */
