#include <stdlib.h>
#include <stdio.h>
#include "util/error.h"
#include "util/misc.h"


void
ufa_error_set(ufa_error_t **error, int code, char *message, ...)
{
    if (error == NULL) {
        return;
    }
    va_list ap;
    va_start(ap, message);
    char *m = ufa_str_vprintf(message, ap);
    va_end(ap);
    ufa_error_t *err = malloc(sizeof *err);
    err->code = code;
    err->message = m;
    *error = err;
}

void
ufa_error_free(ufa_error_t *error)
{
    if (error != NULL) {
        free(error->message);
        free(error);
    }
}

void
ufa_error_abort(ufa_error_t *error)
{
    if (error != NULL) {
        fprintf(stderr, "error: %s\n", error->message);
        abort();
    }
}

void
ufa_error_print(ufa_error_t *error)
{
    if (error != NULL) {
        fprintf(stderr, "error: %s\n", error->message);
    }
}

void
ufa_error_print_and_free(ufa_error_t *error)
{
    ufa_error_print(error);
    ufa_error_free(error);
}