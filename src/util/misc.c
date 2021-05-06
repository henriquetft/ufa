/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Miscellaneous utility functions
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

#include "misc.h"
#include "list.h"
#include "logging.h"

int
ufa_util_str_startswith(const char *str, const char *prefix)
{
    int x = (strstr(str, prefix) == str);
    return x;
}


int
ufa_util_str_endswith(const char *str, const char *suffix)
{
    int p = strlen(str) - strlen(suffix);
    if (p < 0) {
        return 0;
    }
    char *pos = ((char *)str) + p;
    int r = strstr(pos, suffix) == pos;
    return r;
}


static char *
_join_path(const char *delim, const char *first_element, int args, va_list *ap)
{
    char *next = (char *)first_element;
    char *buf = ufa_strdup(next);
    int ends_with_delim = ufa_util_str_endswith(buf, delim);

    for (int x = 0; x < args - 1; x++) {
        /*(next = va_arg(*ap, char*)) != NULL*/
        next = va_arg(*ap, char *);
        if (next == NULL) {
            break;
        }

        if (!ends_with_delim && !ufa_util_str_startswith(next, delim)) {
            char *temp = ufa_util_strcat(buf, delim);
            free(buf);
            buf = temp;

        } else if (ends_with_delim && ufa_util_str_startswith(next, delim)) {
            next = next + strlen(delim);
        }

        char *temp = ufa_util_strcat(buf, next);
        free(buf);
        buf = temp;

        ends_with_delim = ufa_util_str_endswith(buf, delim);
    }
    return buf;
}


char *
ufa_util_join_path(int args, const char *first_element, ...)
{
    va_list ap;

    va_start(ap, first_element);
    /* TODO use the default file separator */
    char *str = _join_path(UFA_FILE_SEPARATOR, first_element, args, &ap);

    va_end(ap);

    return str;
}


char *
ufa_util_join_str(char *delim, int args, const char *first_element, ...)
{
    va_list ap;

    va_start(ap, first_element);
    char *str = _join_path(delim, first_element, args, &ap);

    va_end(ap);

    return str;
}


ufa_list_t *
ufa_util_str_split(const char *str, const char *delim)
{
    ufa_list_t *list = NULL;

    char *s = ufa_strdup(str);

    char *ptr = strtok(s, delim);

    while (ptr != NULL) {
        list = ufa_list_append(list, ufa_strdup(ptr));
        ptr = strtok(NULL, delim);
    }

    free(s);
    return list;
}


char *
ufa_util_strcat(const char *str1, const char *str2)
{
    int len = (strlen(str1) + strlen(str2) + 1);
    char *new_str = malloc(len * sizeof *new_str);

    if (new_str != NULL) {
        snprintf(new_str, len, "%s%s", str1, str2);
    }
    return new_str;
}


int
ufa_util_isdir(const char *filename)
{
    struct stat st;
    return (stat(filename, &st) == 0 && S_ISDIR(st.st_mode));
}


int
ufa_util_isfile(const char *filename)
{
    struct stat st;
    return (stat(filename, &st) == 0 && S_ISREG(st.st_mode));
}

char *
ufa_util_get_current_dir()
{
   char cwd[PATH_MAX];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       return ufa_strdup(cwd);
   } else {
       perror("getcwd() error");
       return NULL;
   }
}


int
ufa_util_strequals(const char *str1, const char *str2)
{
    /* TODO write without strcmp */
    return !strcmp(str1, str2);
}


char *
ufa_strdup(const char *str)
{
    return strdup(str);
}


char *
ufa_util_str_multiply(const char *str, int times)
{
    // if (times < 0) {
    //     return NULL;
    // }
    int len = strlen(str);
    int total = times * len * sizeof(char) + 1;
    char *new_str = (char *)malloc(total);

    strcpy(new_str, "");
    for (int x = 0; x < times; x++) {
        strcat(new_str + len * x, str);
    }
    return new_str;
}

int
ufa_util_strcount(const char *str, const char *part)
{
    size_t len_part = strlen(part);
    int found = 0;
    char *f = str;
    while ((f = strstr(f, part)) != NULL) {
        found++;
        f = f + len_part; 
    }
    return found;
}


char *
ufa_str_vprintf(char const *format, va_list ap)
{
    va_list args2;
    va_copy(args2, ap);
    size_t len = vsnprintf(NULL, 0, format, ap);
    char *buffer = (char*) calloc(len+1, 1);
    vsprintf(buffer, format, args2);
    return buffer;
}

char *
ufa_str_sprintf(char const *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *result = ufa_str_vprintf(format, ap);
    va_end(ap);
    return result;
}