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

ufa_list_t *
ufa_repo_get_tags_for_file(const char *filename);

#endif /* REPO_H_ */