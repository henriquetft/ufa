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

typedef void (*ufa_list_foreach_fn_t)(void *element, void *user_data);
typedef void (*ufa_list_free_fn_t)(void *data);
typedef void *(*ufa_list_cpydata_fn_t)(const void *data);
typedef bool (*ufa_list_equal_fn_t)(const void *element1, const void *element2);

/** Represents a node of the linked list */
struct ufa_list {
	void *data;
	struct ufa_list *next;
	struct ufa_list *prev;
	ufa_list_free_fn_t free_func;

};

/** Macro to check if this list contains the specified element */
#define ufa_list_contains(list, data, eqfunc)                                  \
	(ufa_list_find_by_data(list,                                           \
			       data,                                           \
			       (ufa_list_equal_fn_t) eqfunc)                   \
	 != NULL)

/** Macro to rewind a list back to the first element */
#define UFA_LIST_REWIND(list)                                                  \
	for (; (list->prev != NULL); list = list->prev)                        \
		;

/** Macro to use with FOR statement to iterate over an ufa_list */
#define UFA_LIST_EACH(iter_var, list_var)                                      \
	struct ufa_list *iter_var = list_var;                                  \
	(iter_var != NULL);                                                    \
	iter_var = iter_var->next

/** Macro to print to stdout all elements of the list */
#define UFA_LIST_PRINTSTR(list, prefix)                                        \
	for (UFA_LIST_EACH(i, list)) {                                         \
		printf("%s%s\n", prefix, (char *) i->data);                    \
	}

/**
 * Add a new element on to the end of the list
 * @param list A pointer to list
 * @param element Element
 * @return The same list with the new element in the tail, or the new node
 *         (new single-element list) if list was NULL.
 */
struct ufa_list *ufa_list_append(struct ufa_list *list, const void *element);


/**
 * Add a new element on to the end of the list and set the free function for
 * element.
 * @param list A pointer to list
 * @param element Element
 * @param free_func Function to free element.
 * @return The same list with the new element in the tail, or the new node
 *         (new single-element list) if list was NULL.
 */
struct ufa_list *ufa_list_append2(struct ufa_list *list,
				  const void *element,
				  ufa_list_free_fn_t free_func);


/**
 * Prepend element on to the start of the list.
 * @param list  A pointer to list
 * @param element Element
 * @return New node (head of the list)
 */
struct ufa_list *ufa_list_prepend(struct ufa_list *list, const void *element);


/**
 * Prepend element on to the start of the list and set the free function for
 * element.
 * @param list  A pointer to list
 * @param element Element
 * @return New node (head of the list)
 */
struct ufa_list *ufa_list_prepend2(struct ufa_list *list,
				   const void *element,
				   ufa_list_free_fn_t free_func);

struct ufa_list *ufa_list_insert(struct ufa_list *list,
				 unsigned int pos,
				 const void *element);

struct ufa_list *ufa_list_concat(struct ufa_list *list, struct ufa_list *list2);

struct ufa_list *ufa_list_get(struct ufa_list *list, unsigned int pos);

struct ufa_list *ufa_list_get_first(struct ufa_list *list);

struct ufa_list *ufa_list_get_last(struct ufa_list *list);

unsigned int ufa_list_size(const struct ufa_list *list);

void ufa_list_foreach(struct ufa_list *list, ufa_list_foreach_fn_t forfunc,
		      void *user_data);

struct ufa_list *ufa_list_unlink_node(struct ufa_list *list,
				      struct ufa_list *node);

void ufa_list_free_full(struct ufa_list *list, ufa_list_free_fn_t freefunc);

struct ufa_list * ufa_list_find_by_data(const struct ufa_list *list,
				       const void *data,
				       ufa_list_equal_fn_t eq_func);

struct ufa_list *ufa_list_clone(struct ufa_list *list,
				ufa_list_cpydata_fn_t copyfunc,
				ufa_list_free_fn_t freefunc);

struct ufa_list *ufa_list_reverse(struct ufa_list *list);


void ufa_list_free(struct ufa_list *list);

#endif /* UFA_LIST_H_ */
