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
} ufa_repo_error_t;


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

#endif /* REPO_H_ */