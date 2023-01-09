/* ========================================================================== */
/* Copyright (c) 2021 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for doubly linked list                                         */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#ifndef UFA_LIST_H_
#define UFA_LIST_H_

#include <stdbool.h>

/** Represents a node of the linked list */
struct ufa_list {
	void *data;
	struct ufa_list *next;
	struct ufa_list *prev;
};

typedef void (*ufa_list_for_each_func)(void *element, void *user_data);
typedef void (*ufa_list_free_func)(void *data);

#define ufa_list_contains(list, data, eqfunc)                                  \
	(ufa_list_find_by_data(list, data, eqfunc) != NULL)

/** Macro to rewind a list back to the first element */
#define UFA_LIST_REWIND(list)                                                  \
	for (; (list->prev != NULL); list = list->prev)                        \
		;

/** Macro to use with FOR statement to iterate over an ufa_list */
#define UFA_LIST_EACH(iter_var, list_var)                                      \
	struct ufa_list *iter_var = list_var;                                  \
	(iter_var != NULL);                                                    \
	iter_var = iter_var->next

struct ufa_list *ufa_list_append(struct ufa_list *list, void *element);

struct ufa_list *ufa_list_prepend(struct ufa_list *list, void *element);

struct ufa_list *ufa_list_insert(struct ufa_list *list, unsigned int pos,
				 void *element);

struct ufa_list *ufa_list_concat(struct ufa_list *list, struct ufa_list *list2);

struct ufa_list *ufa_list_get(struct ufa_list *list, unsigned int pos);

struct ufa_list *ufa_list_get_first(struct ufa_list *list);

struct ufa_list *ufa_list_get_last(struct ufa_list *list);

unsigned int ufa_list_size(struct ufa_list *list);

void ufa_list_foreach(struct ufa_list *list, ufa_list_for_each_func forfunc,
		      void *user_data);

struct ufa_list *ufa_list_unlink_node(struct ufa_list *list,
				      struct ufa_list *node);

void ufa_list_free_full(struct ufa_list *list, ufa_list_free_func freefunc);

struct ufa_list *
ufa_list_find_by_data(const struct ufa_list *list, const void *data,
		      bool (*eq_func)(void *element, void *user_data));

void ufa_list_free(struct ufa_list *list);

#endif /* UFA_LIST_H_ */
