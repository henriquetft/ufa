/* ========================================================================== */
/* Copyright (c) 2022 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of data functions (data.h).                                 */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/data.h"
#include "util/hashtable.h"
#include "core/config.h"
#include "core/repo.h"
#include "util/misc.h"
#include "util/string.h"
#include "util/logging.h"
#include <stdio.h>
#include <stdbool.h>


/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

/** Maps WD -> filename */
static ufa_hashtable_t *repos = NULL;

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void add_set(ufa_hashtable_t *set, const char *str);
static void  init_repo_hashtable();
static ufa_repo_t *get_repo_for(const char *filepath, struct ufa_error **error);
static ufa_repo_t *get_repo(const char *repodir, struct ufa_error **error);

/* ========================================================================== */
/* FUNCTIONS FROM data.h                                                      */
/* ========================================================================== */

bool ufa_data_init_repo(const char *repository, struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo(repository, error);
	return repo != NULL;
}

void ufa_data_close()
{
	ufa_hashtable_free(repos);
}

bool ufa_data_gettags(const char *filepath,
		      struct ufa_list **list,
		      struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_gettags(repo, filepath, list, error);
}

bool ufa_data_settag(const char *filepath,
		     const char *tag,
		     struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_settag(repo, filepath, tag, error);
}

bool ufa_data_unsettag(const char *filepath,
		       const char *tag,
		       struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_unsettag(repo, filepath, tag, error);
}

bool ufa_data_cleartags(const char *filepath,
			struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_cleartags(repo, filepath, error);
}


int ufa_data_inserttag(const char *repodir,
		       const char *tag,
		       struct ufa_error **error)
{
	int ret = -1;
	ufa_repo_t* repo = get_repo(repodir, error);
	if (repo == NULL) {
		goto end;
	}
	ret = ufa_repo_inserttag(repo, tag, error);
end:
	return ret;
}


struct ufa_list *ufa_data_listtags(const char *repodir,
				   struct ufa_error **error)
{
	struct ufa_list *ret = NULL;

	ufa_repo_t* repo = get_repo(repodir, error);
	if (repo == NULL) {
		goto end;
	}
	ret = ufa_repo_listtags(repo, error);
end:
	return ret;
}


bool ufa_data_setattr(const char *filepath,
		      const char *attribute,
		      const char *value,
		      struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_setattr(repo, filepath, attribute, value, error);
}

bool ufa_data_unsetattr(const char *filepath,
			const char *attribute,
			struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_unsetattr(repo, filepath, attribute, error);
}

// returns list of ufa_repo_attr_t
struct ufa_list *ufa_data_getattr(const char *filepath,
				  struct ufa_error **error)
{
	struct ufa_list *ret = NULL;

	ufa_repo_t *repo = get_repo_for(filepath, error);
	if (repo == NULL) {
		goto end;
	}
	ret = ufa_repo_getattr(repo, filepath, error);
end:
	return ret;
}


struct ufa_list *ufa_data_search(struct ufa_list *repo_dirs,
				 struct ufa_list *filter_attr,
				 struct ufa_list *tags,
				 bool include_repo_from_config,
				 struct ufa_error **error)
{
	ufa_debug(__func__);

	struct ufa_list *list_repo = NULL;
	struct ufa_list *ret = NULL;
	struct ufa_list *result_tmp = NULL;

	ufa_hashtable_t *set = UFA_HASHTABLE_STRING();
	for (UFA_LIST_EACH(i, repo_dirs)) {
		char *str = (char *) i->data;
		if (ufa_repo_isrepo(str)) {
			add_set(set, str);
		} else {
			ufa_error("'%s' is not a repository", str);
		}
	}

	ufa_debug("Include repo from config? %d", include_repo_from_config);
	if (include_repo_from_config) {
		struct ufa_list *list_dirs_cfg = ufa_config_dirs(false, error);
		if (list_dirs_cfg == NULL && *error) {
			goto end;
		}
		for (UFA_LIST_EACH(i, list_dirs_cfg)) {
			add_set(set, i->data);
		}
		ufa_list_free(list_dirs_cfg);
	}

	list_repo = ufa_hashtable_keys(set);

	for (UFA_LIST_EACH(i, list_repo)) {
		char *repo_folder = (char *) i->data;
		ufa_debug("Searching in: %s", repo_folder);
		ufa_repo_t *repo = get_repo(repo_folder, error);
		if_goto(*error, end);

		result_tmp = ufa_repo_search(repo, filter_attr, tags, error);
		if_goto(*error, end);

		char *repo_path = ufa_repo_getrepopath(repo);

		// concatenate repo_path
		for (UFA_LIST_EACH(i, result_tmp)) {
			char *file = (char *) i->data;
			ret = ufa_list_append2(
			    ret, ufa_util_joinpath(repo_path, file, NULL),
			    ufa_free);
		}

		// free resources
		ufa_free(repo_path);
		ufa_list_free(result_tmp);
		result_tmp = NULL;
	}

end:
	ufa_list_free(list_repo);
	ufa_list_free_full(result_tmp, ufa_free);
	ufa_hashtable_free(set);
	return ret;
}

bool ufa_data_removefile(const char *filepath, struct ufa_error **error)
{
	char *dir = ufa_util_dirname(filepath);
	ufa_repo_t *repo = get_repo_for(dir, error);
	ufa_free(dir);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_removefile(repo, filepath, error);
}

bool ufa_data_renamefile(const char *oldfilepath,
			 const char *newfilepath,
			 struct ufa_error **error)
{
	ufa_repo_t *repo = get_repo_for(newfilepath, error);
	if (repo == NULL) {
		return false;
	}
	return ufa_repo_renamefile(repo, oldfilepath, newfilepath, error);
}


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static void add_set(ufa_hashtable_t *set, const char *str)
{
	ufa_debug("Adding repo to search: %s", str);
	char *filepath = ufa_util_abspath(str);
	if (filepath == NULL) {
		// FIXME handle with ufa_error
		perror(STR_NOTNULL(str));
		return;
	}

	if (ufa_hashtable_has_key(set, filepath)) {
		ufa_free(filepath);
	} else {
		ufa_hashtable_put(set, filepath, NULL);
	}
}


static void init_repo_hashtable()
{
	if (repos == NULL) {
		repos = ufa_hashtable_new((ufa_hash_fn_t) ufa_str_hash,
					  (ufa_hash_equal_fn_t) ufa_str_equals,
					  ufa_free,
					  (ufa_hash_free_fn_t) ufa_repo_free);
	}
}


static ufa_repo_t *get_repo(const char *repodir, struct ufa_error **error)
{
	if (!repodir) {
		return NULL; // TODO is this correct ?
	}
	init_repo_hashtable();
	ufa_repo_t *repo = (ufa_repo_t *) ufa_hashtable_get(repos, repodir);
	if (repo == NULL) {
		repo = ufa_repo_init(repodir, error);
		ufa_hashtable_put(repos, ufa_str_dup(repodir), repo);
	}
	return repo;
}


static ufa_repo_t *get_repo_for(const char *filepath, struct ufa_error **error)
{
	char *s = ufa_repo_getrepofolderfor(filepath, error);
	ufa_repo_t *ret = get_repo(s, error);
	ufa_free(s);
	return ret;
}