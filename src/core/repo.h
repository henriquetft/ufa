#ifndef REPO_H_
#define REPO_H_

#include <stdbool.h>
#include "util/list.h"
#include "util/error.h"


typedef enum 
{
    UFA_ERROR_DATABASE,
    UFA_ERROR_NOTDIR,
    UFA_ERROR_FILE,
    UFA_ERROR_STATE,
    UFA_ERROR_ARGS,
} ufa_repo_error_t;

typedef enum 
{
     UFA_REPO_EQUAL = 0,     // =
     UFA_REPO_WILDCARD,      // ~=
     UFA_REPO_MATCH_MODE_TOTAL,
} ufa_repo_match_mode_t;


/* List of supported match modes */
extern const ufa_repo_match_mode_t ufa_repo_match_mode_supported[];


typedef struct ufa_repo_filter_attr_s {
    char *attribute;
    char *value;
    ufa_repo_match_mode_t match_mode;
} ufa_repo_filter_attr_t;

typedef struct ufa_repo_attr_s {
    char *attribute;
    char *value;
} ufa_repo_attr_t;

char *
ufa_repo_get_filename(const char *filepath);

bool
ufa_repo_init(const char *repository, ufa_error_t **error);

ufa_list_t *
ufa_get_all_tags(ufa_error_t **error);

bool
ufa_repo_is_a_tag(const char *path, ufa_error_t** error);

ufa_list_t *
ufa_repo_list_files_for_dir(const char *path, ufa_error_t **error);

char *
ufa_repo_get_file_path(const char *path, ufa_error_t **error);

bool
ufa_repo_get_tags_for_file(const char *filename, ufa_list_t **list, ufa_error_t **error);

bool
ufa_repo_set_tag_on_file(const char *filename, const char *tag, ufa_error_t **error);

bool
ufa_repo_clear_tags_for_file(const char *filename, ufa_error_t **error);

bool
ufa_repo_unset_tag_on_file(const char *filepath, const char *tag, ufa_error_t **error);

int
ufa_repo_insert_tag(const char *tag, ufa_error_t **error);

ufa_repo_filter_attr_t *
ufa_repo_filter_attr_new(const char *attribute, const char *value, ufa_repo_match_mode_t match_mode);

void
ufa_repo_filter_attr_free(ufa_repo_filter_attr_t *filter);

ufa_list_t *
ufa_repo_search(ufa_list_t *filter_attr, ufa_list_t *tags, ufa_error_t **error);

bool
ufa_repo_set_attr(const char *filepath, const char *attribute, const char *value, ufa_error_t **error);

bool
ufa_repo_unset_attr(const char *filepath, const char *attribute, ufa_error_t **error);

// returns list of ufa_repo_attr_t
ufa_list_t *
ufa_repo_get_attr(const char *filepath, ufa_error_t **error);

void
ufa_repo_attr_free(ufa_repo_attr_t *attr);








#endif /* REPO_H_ */