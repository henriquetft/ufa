#ifndef UFA_ERROR_H_
#define UFA_ERROR_H_

#include <stdarg.h>

struct ufa_error
{
    int code;
    char *message;
};

void
ufa_error_set(struct ufa_error **error, int code, char *format, ...);

void
ufa_error_free(struct ufa_error *error);

void
ufa_error_abort(struct ufa_error *error);

void
ufa_error_print(struct ufa_error *error);

void
ufa_error_print_and_free(struct ufa_error *error);

#endif /* UFA_ERROR_H_ */