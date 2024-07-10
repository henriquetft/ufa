/* ========================================================================== */
/* Copyright (c) 2021-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for repo module (repo.h)                                       */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_REPO_H_
#define UFA_REPO_H_

#include "core/errors.h"
#include "util/error.h"
#include "util/list.h"
#include <stdbool.h>


/* List of supported match modes */
typedef struct ufa_repo ufa_repo_t;

enum ufa_repo_matchmode {
	UFA_REPO_EQUAL = 0,       // =
	UFA_REPO_WILDCARD,        // ~= (accept *)
	UFA_REPO_MATCHMODE_TOTAL,
};

struct ufa_repo_filterattr {
	char *attribute;
	char *value;
	enum ufa_repo_matchmode matchmode;
};

struct ufa_repo_attr {
	char *attribute;
	char *value;
};

extern const enum ufa_repo_matchmode ufa_repo_matchmode_supported[];


ufa_repo_t *ufa_repo_init(const char *repository, struct ufa_error **error);

char *ufa_repo_getrepopath(const ufa_repo_t *repo);


struct ufa_list *ufa_repo_listtags(const ufa_repo_t *repo, struct ufa_error
**error);

struct ufa_list *ufa_repo_listfiles(const ufa_repo_t *repo,
				    const char *dirpath,
				    struct ufa_error **error);

bool ufa_repo_gettags(const ufa_repo_t *repo,
		      const char *filename,
		      struct ufa_list **list,
		      struct ufa_error **error);

bool ufa_repo_settag(const ufa_repo_t *repo,
		     const char *filepath,
		     const char *tag,
		     struct ufa_error **error);

bool ufa_repo_cleartags(const ufa_repo_t *repo,
			const char *filepath,
			struct ufa_error **error);

bool ufa_repo_unsettag(const ufa_repo_t *repo,
		       const char *filepath,
		       const char *tag,
		       struct ufa_error **error);

int ufa_repo_inserttag(const ufa_repo_t *repo,
		       const char *tag,
		       struct ufa_error **error);


struct ufa_list *ufa_repo_search(const ufa_repo_t *repo,
				 struct ufa_list *filter_attr,
				 struct ufa_list *tags,
				 struct ufa_error **error);

bool ufa_repo_setattr(const ufa_repo_t *repo,
		      const char *filepath,
		      const char *attribute,
		      const char *value,
		      struct ufa_error **error);

bool ufa_repo_unsetattr(const ufa_repo_t *repo,
			const char *filepath,
			const char *attribute,
			struct ufa_error **error);


/**
 *
 * @param repo
 * @param filepath
 * @param error
 * @return List of struct ufa_repo_attr
 */
struct ufa_list *ufa_repo_getattr(const ufa_repo_t *repo,
				  const char *filepath,
				  struct ufa_error **error);

/**
 * Checks whether a path is a tag.
 * E.g.: /tag1/tag2/tag3 or /tag1/tag2/file.
 *
 * @param repo
 * @param path
 * @param error
 * @return
 */
bool ufa_repo_isatag(const ufa_repo_t *repo,
		     const char *path,
		     struct ufa_error **error);

// FIXME
char *ufa_repo_get_realfilepath(const ufa_repo_t *repo,
				const char *path,
				struct ufa_error **error);

struct ufa_repo_filterattr *
ufa_repo_filterattr_new(const char *attribute,
			const char *value,
			enum ufa_repo_matchmode match_mode);

void ufa_repo_filterattr_free(struct ufa_repo_filterattr *filter);


void ufa_repo_attr_free(struct ufa_repo_attr *attr);

char *ufa_repo_getrepofolderfor(const char *filepath, struct ufa_error **error);

void ufa_repo_free(ufa_repo_t *repo);


bool ufa_repo_isrepo(char *directory);

bool ufa_repo_removefile(const ufa_repo_t *repo,
			 const char *filepath,
			 struct ufa_error **error);

bool ufa_repo_renamefile(const ufa_repo_t *repo,
			 const char *oldfilepath,
			 const char *newfilepath,
			 struct ufa_error **error);

#endif /* UFA_REPO_H_ */