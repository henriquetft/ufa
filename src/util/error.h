#ifndef UFA_ERROR_H_
#define UFA_ERROR_H_

#include <stdarg.h>

struct ufa_error {
	int code;
	char *message;
};

void ufa_error_new(struct ufa_error **error, int code, char *format, ...);

void ufa_error_free(struct ufa_error *error);

void ufa_error_abort(const struct ufa_error *error);

void ufa_error_print_prefix(const struct ufa_error *error, const char *prefix);

void ufa_error_print(const struct ufa_error *error);

void ufa_error_print_and_free(struct ufa_error *error);

void ufa_error_print_and_free_prefix(struct ufa_error *error, const char *prefix);

#endif /* UFA_ERROR_H_ */