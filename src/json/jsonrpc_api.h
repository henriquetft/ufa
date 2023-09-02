#ifndef UFA_JSONRPC_API_H_
#define UFA_JSONRPC_API_H_

#include "util/error.h"
#include <stdbool.h>

typedef struct ufa_jsonrpc_api ufa_jsonrpc_api_t;


ufa_jsonrpc_api_t *ufa_jsonrpc_api_init(struct ufa_error **error);

void ufa_jsonrpc_api_close(ufa_jsonrpc_api_t *api, struct ufa_error **error);


bool ufa_jsonrpc_api_settag(ufa_jsonrpc_api_t *api,
			    const char *filepath,
			    const char *tag,
			    struct ufa_error **error);

struct ufa_list *ufa_jsonrpc_api_listtags(ufa_jsonrpc_api_t *api,
					  const char *repodir,
					  struct ufa_error **error);

bool ufa_jsonrpc_api_gettags(ufa_jsonrpc_api_t *api,
			     const char *filepath,
			     struct ufa_list **list,
			     struct ufa_error **error);

int ufa_jsonrpc_api_inserttag(ufa_jsonrpc_api_t *api,
			      const char *repodir,
			      const char *tag,
			      struct ufa_error **error);

bool ufa_jsonrpc_api_cleartags(ufa_jsonrpc_api_t *api,
			const char *filepath,
			struct ufa_error **error);

bool ufa_jsonrpc_api_unsettag(ufa_jsonrpc_api_t *api,
		       const char *filepath,
		       const char *tag,
		       struct ufa_error **error);

bool ufa_jsonrpc_api_setattr(ufa_jsonrpc_api_t *api,
			     const char *filepath,
			     const char *attribute,
			     const char *value,
			     struct ufa_error **error);

struct ufa_list *ufa_jsonrpc_api_getattr(ufa_jsonrpc_api_t *api,
					 const char *filepath,
					 struct ufa_error **error);

bool ufa_jsonrpc_api_unsetattr(ufa_jsonrpc_api_t *api,
			       const char *filepath,
			       const char *attribute,
			       struct ufa_error **error);

struct ufa_list *ufa_jsonrpc_api_search(ufa_jsonrpc_api_t *api,
					struct ufa_list *repo_dirs,
					struct ufa_list *filter_attr,
					struct ufa_list *tags,
					bool include_repo_from_config,
					struct ufa_error **error);

#endif // UFA_JSONRPC_API_H_
