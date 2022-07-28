#include "util/error.h"
#include "util/misc.h"
#include <stdio.h>
#include <stdlib.h>

void ufa_error_new(struct ufa_error **error, int code, char *format, ...)
{
	if (error == NULL) {
		return;
	}
	va_list ap;
	va_start(ap, format);
	char *m = ufa_str_vprintf(format, ap);
	va_end(ap);
	struct ufa_error *err = malloc(sizeof *err);
	err->code = code;
	err->message = m;
	*error = err;
}

void ufa_error_free(struct ufa_error *error)
{
	if (error != NULL) {
		free(error->message);
		free(error);
	}
}

void ufa_error_abort(const struct ufa_error *error)
{
	if (error != NULL) {
		fprintf(stderr, "error: %s\n", error->message);
		fflush(stderr);
		abort();
	}
}

void ufa_error_print_prefix(const struct ufa_error *error, const char *prefix)
{
	if (error != NULL) {
		fprintf(stderr, "%s: %s\n", prefix, error->message);
	}
}

void ufa_error_print(const struct ufa_error *error)
{
	ufa_error_print_prefix(error, "error");
}

void ufa_error_print_and_free(struct ufa_error *error)
{
	ufa_error_print(error);
	ufa_error_free(error);
}

void ufa_error_print_and_free_prefix(struct ufa_error *error,
				     const char *prefix)
{
	ufa_error_print_prefix(error, prefix);
	ufa_error_free(error);
}