#ifndef UFA_ERROR_H_
#define UFA_ERROR_H_

#include <stdarg.h>

typedef struct ufa_error_s
{
    int code;
    char *message;
} ufa_error_t;

void
ufa_error_set(ufa_error_t **error, int code, char *format, ...);

void
ufa_error_free(ufa_error_t *error);

void
abort_on_error(ufa_error_t *error);

#endif /* UFA_ERROR_H_ */