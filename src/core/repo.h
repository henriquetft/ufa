#ifndef REPO_H_
#define REPO_H_

#include <stdbool.h>
#include "util/list.h"
#include "util/error.h"


enum ufa_repo_error
{
    UFA_ERROR_DATABASE,
    UFA_ERROR_NOTDIR,
    UFA_ERROR_FILE,
    UFA_ERROR_STATE,
    UFA_ERROR_ARGS,
};

enum ufa_repo_match_mode
{
     UFA_REPO_EQUAL = 0,     // =
     UFA_REPO_WILDCARD,      // ~=
     UFA_REPO_MATCH_MODE_TOTAL,
};


/* List of supported match modes */
extern const enum ufa_repo_match_mode ufa_repo_match_mode_supported[];


struct ufa_repo_filter_attr {
    char *attribute;
    char *value;
    enum ufa_repo_match_mode match_mode;
};

struct ufa_repo_attr {
    char *attribute;
    char *value;
};

char *
ufa_repo_get_filename(const char *filepath);

bool
ufa_repo_init(const char *repository, struct ufa_error **error);

ufa_list_t *
ufa_get_all_tags(struct ufa_error **error);

bool
ufa_repo_is_a_tag(const char *path, struct ufa_error** error);

ufa_list_t *
ufa_repo_list_files_for_dir(const char *path, struct ufa_error **error);

char *
ufa_repo_get_file_path(const char *path, struct ufa_error **error);

bool
ufa_repo_get_tags_for_file(const char *filename, ufa_list_t **list, struct ufa_error **error);

bool
ufa_repo_set_tag_on_file(const char *filename, const char *tag, struct ufa_error **error);

bool
ufa_repo_clear_tags_for_file(const char *filename, struct ufa_error **error);

bool
ufa_repo_unset_tag_on_file(const char *filepath, const char *tag, struct ufa_error **error);

int
ufa_repo_insert_tag(const char *tag, struct ufa_error **error);

struct ufa_repo_filter_attr *
ufa_repo_filter_attr_new(const char *attribute, const char *value, enum ufa_repo_match_mode match_mode);

void
ufa_repo_filter_attr_free(struct ufa_repo_filter_attr *filter);

ufa_list_t *
ufa_repo_search(ufa_list_t *filter_attr, ufa_list_t *tags, struct ufa_error **error);

bool
ufa_repo_set_attr(const char *filepath, const char *attribute, const char *value, struct ufa_error **error);

bool
ufa_repo_unset_attr(const char *filepath, const char *attribute, struct ufa_error **error);

// returns list of ufa_repo_attr_t
ufa_list_t *
ufa_repo_get_attr(const char *filepath, struct ufa_error **error);

void
ufa_repo_attr_free(struct ufa_repo_attr *attr);








#endif /* REPO_H_ */