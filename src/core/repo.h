#ifndef UFA_REPO_H_
#define UFA_REPO_H_

#include "util/error.h"
#include "util/list.h"
#include <stdbool.h>

enum ufa_repo_error {
	UFA_ERROR_DATABASE       = 0,
	UFA_ERROR_NOTDIR         = 1,
	UFA_ERROR_FILE           = 2,
	UFA_ERROR_FILE_NOT_IN_DB = 3,
	UFA_ERROR_ARGS           = 4,
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

typedef struct ufa_repo ufa_repo_t;


ufa_repo_t *ufa_repo_init(const char *repository, struct ufa_error **error);

char *ufa_repo_getrepopath(ufa_repo_t *repo);


struct ufa_list *ufa_repo_listtags(ufa_repo_t *repo, struct ufa_error **error);

struct ufa_list *ufa_repo_listfiles(ufa_repo_t *repo,
				    const char *dirpath,
				    struct ufa_error **error);

bool ufa_repo_gettags(ufa_repo_t *repo,
		      const char *filename,
		      struct ufa_list **list,
		      struct ufa_error **error);

bool ufa_repo_settag(ufa_repo_t *repo,
		     const char *filepath,
		     const char *tag,
		     struct ufa_error **error);

bool ufa_repo_cleartags(ufa_repo_t *repo,
			const char *filepath,
			struct ufa_error **error);

bool ufa_repo_unsettag(ufa_repo_t *repo,
		       const char *filepath,
		       const char *tag,
		       struct ufa_error **error);

int ufa_repo_inserttag(ufa_repo_t *repo,
		       const char *tag,
		       struct ufa_error **error);


struct ufa_list *ufa_repo_search(ufa_repo_t *repo,
				 struct ufa_list *filter_attr,
				 struct ufa_list *tags,
				 struct ufa_error **error);

bool ufa_repo_setattr(ufa_repo_t *repo,
		      const char *filepath,
		      const char *attribute,
		      const char *value,
		      struct ufa_error **error);

bool ufa_repo_unsetattr(ufa_repo_t *repo,
			const char *filepath,
			const char *attribute,
			struct ufa_error **error);

// returns list of ufa_repo_attr_t
struct ufa_list *ufa_repo_getattr(ufa_repo_t *repo,
				  const char *filepath,
				  struct ufa_error **error);

bool ufa_repo_isatag(ufa_repo_t *repo, const char *path,
		     struct ufa_error **error);

// FIXME
char *ufa_repo_get_realfilepath(ufa_repo_t *repo, const char *path,
				struct ufa_error **error);

struct ufa_repo_filterattr *
ufa_repo_filterattr_new(const char *attribute,
			const char *value,
			enum ufa_repo_matchmode match_mode);

void ufa_repo_filterattr_free(struct ufa_repo_filterattr *filter);

void ufa_repo_attrfree(struct ufa_repo_attr *attr);

char *ufa_repo_getrepofolderfor(const char *filepath, struct ufa_error **error);

void ufa_repo_free(ufa_repo_t *repo);

bool ufa_repo_isrepo(char *directory);

bool ufa_repo_removefile(ufa_repo_t *repo, char *filepath,
			 struct ufa_error **error);

bool ufa_repo_renamefile(ufa_repo_t *repo, char *oldfilepath, char *newfilepath,
			 struct ufa_error **error);

#endif /* UFA_REPO_H_ */