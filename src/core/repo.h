#ifndef UFA_REPO_H_
#define UFA_REPO_H_

#include "util/error.h"
#include "util/list.h"
#include <stdbool.h>

enum ufa_repo_error {
	UFA_ERROR_DATABASE,
	UFA_ERROR_NOTDIR,
	UFA_ERROR_FILE,
	UFA_ERROR_STATE,
	UFA_ERROR_ARGS,
};

enum ufa_repo_matchmode {
	UFA_REPO_EQUAL = 0, // =
	UFA_REPO_WILDCARD,  // ~=
	UFA_REPO_MATCHMODE_TOTAL,
};

/* List of supported match modes */
extern const enum ufa_repo_matchmode ufa_repo_matchmode_supported[];

struct ufa_repo_filterattr {
	char *attribute;
	char *value;
	enum ufa_repo_matchmode matchmode;
};

struct ufa_repo_attr {
	char *attribute;
	char *value;
};


bool ufa_repo_init(const char *repository, struct ufa_error **error);

struct ufa_list *ufa_listtags(struct ufa_error **error);

bool ufa_repo_isatag(const char *path, struct ufa_error **error);

struct ufa_list *ufa_repo_listfiles(const char *dirpath,
				    struct ufa_error **error);

char *ufa_repo_get_realfilepath(const char *path, struct ufa_error **error);

bool ufa_repo_gettags(const char *filename,
		      struct ufa_list **list,
		      struct ufa_error **error);

bool ufa_repo_settag(const char *filepath,
		     const char *tag,
		     struct ufa_error **error);

bool ufa_repo_cleartags(const char *filepath,
			struct ufa_error **error);

bool ufa_repo_unsettag(const char *filepath, const char *tag,
		       struct ufa_error **error);

int ufa_repo_inserttag(const char *tag, struct ufa_error **error);

struct ufa_repo_filterattr *
ufa_repo_filterattr_new(const char *attribute,
			const char *value,
			enum ufa_repo_matchmode match_mode);

void ufa_repo_filterattr_free(struct ufa_repo_filterattr *filter);

struct ufa_list *ufa_repo_search(struct ufa_list *filter_attr,
				 struct ufa_list *tags,
				 struct ufa_error **error);

bool ufa_repo_setattr(const char *filepath, const char *attribute,
		      const char *value, struct ufa_error **error);

bool ufa_repo_unsetattr(const char *filepath, const char *attribute,
			struct ufa_error **error);

// returns list of ufa_repo_attr_t
struct ufa_list *ufa_repo_getattr(const char *filepath,
				  struct ufa_error **error);

void ufa_repo_attrfree(struct ufa_repo_attr *attr);

#endif /* UFA_REPO_H_ */