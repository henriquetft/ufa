#ifndef REPO_H_
#define REPO_H_

#include "list.h"

int
ufa_repo_init(char *repository);

int
ufa_repo_is_a_tag(const char *path);

char *
ufa_repo_get_file_path(const char *path);

ufa_list_t *
ufa_repo_list_files_for_dir(const char *path);

char *
ufa_repo_get_file_path(const char *path);

int
ufa_repo_get_tags_for_file(const char *filename, ufa_list_t **list);

int
ufa_repo_set_tag_on_file(const char *filename, const char *tag);

int
ufa_repo_clear_tags_for_file(const char *filename);

int
ufa_repo_unset_tag_on_file(const char *filepath, const char *tag);

#endif /* REPO_H_ */