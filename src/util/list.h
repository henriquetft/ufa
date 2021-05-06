#ifndef UFA_LIST_H_
#define UFA_LIST_H_

typedef struct ufa_list_s {
    void *data;
    struct ufa_list_s *next;
    struct ufa_list_s *prev;
} ufa_list_t;

typedef void (*ufa_list_for_each_func)(void *element, void *user_data);
typedef void (*ufa_list_free_func)(void *data);

#define UFA_LIST_REWIND(list)                                                                      \
    for (; (list->prev != NULL); list = list->prev)                                                \
        ;

#define UFA_LIST_EACH(iter_var, list_var)                                                          \
    ufa_list_t *iter_var = list_var;                                                               \
    (iter_var != NULL);                                                                            \
    iter_var = iter_var->next

ufa_list_t *
ufa_list_append(ufa_list_t *list, void *element);

ufa_list_t *
ufa_list_prepend(ufa_list_t *list, void *element);

ufa_list_t *
ufa_list_insert(ufa_list_t *list, unsigned int pos, void *element);

ufa_list_t *
ufa_list_get(ufa_list_t *list, unsigned int pos);

ufa_list_t *
ufa_list_get_first(ufa_list_t *list);

ufa_list_t *
ufa_list_get_last(ufa_list_t *list);

unsigned int
ufa_list_size(ufa_list_t *list);

void
ufa_list_foreach(ufa_list_t *list, ufa_list_for_each_func forfunc, void *user_data);

ufa_list_t *
ufa_list_unlink_node(ufa_list_t *list, ufa_list_t *node);

void
ufa_list_free_full(ufa_list_t *list, ufa_list_free_func freefunc);

void
ufa_list_free(ufa_list_t *list);

#endif /* UFA_LIST_H_ */
