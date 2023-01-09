/* ========================================================================== */
/* Copyright (c) 2022 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Functions that allows the user to interact with repositories               */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#ifndef UFA_DATA_H_
#define UFA_DATA_H_

#include "util/error.h"
#include "util/list.h"
#include <stdbool.h>

void ufa_data_close();


struct ufa_list *ufa_data_listtags(const char *repodir,
				   struct ufa_error **error);

struct ufa_list *ufa_data_listfiles(const char *dirpath,
				    struct ufa_error **error);

bool ufa_data_gettags(const char *filepath,
		      struct ufa_list **list,
		      struct ufa_error **error);

bool ufa_data_settag(const char *filepath,
		     const char *tag,
		     struct ufa_error **error);

bool ufa_data_cleartags(const char *filepath,
			struct ufa_error **error);

bool ufa_data_unsettag(const char *filepath,
		       const char *tag,
		       struct ufa_error **error);

int ufa_data_inserttag(const char *repodir,
		       const char *tag,
		       struct ufa_error **error);


struct ufa_list *ufa_data_search(struct ufa_list *repo_dirs,
				 struct ufa_list *filter_attr,
				 struct ufa_list *tags,
				 bool include_repo_from_config,
				 struct ufa_error **error);

bool ufa_data_setattr(const char *filepath,
		      const char *attribute,
		      const char *value,
		      struct ufa_error **error);

bool ufa_data_unsetattr(const char *filepath,
			const char *attribute,
			struct ufa_error **error);

// returns list of ufa_repo_attr_t
struct ufa_list *ufa_data_getattr(const char *filepath,
				  struct ufa_error **error);

bool ufa_data_removefile(char *filepath, struct ufa_error **error);

bool ufa_data_renamefile(char *oldfilepath, char *newfilepath,
			 struct ufa_error **error);

#endif /* UFA_DATA_H_ */
