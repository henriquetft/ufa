/* ========================================================================== */
/* Copyright (c) 2021-2023 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Doubly linked list (implementation of list.h)                              */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "list.h"
#include "misc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void link_before(struct ufa_list *list, struct ufa_list *newnode);

static struct ufa_list *new_node(const void *data,
				 struct ufa_list *prev,
				 struct ufa_list *next,
				 ufa_list_free_fn_t free_func);

/* ========================================================================== */
/* FUNCTIONS FROM list.h                                                      */
/* ========================================================================== */

/**
 * Adds the element after the last one.
 * Returns: the same list with the new element in the tail, or the new node
 * (new single-element list) if list was NULL
 */
struct ufa_list *ufa_list_append(struct ufa_list *list, const void *element)
{
	struct ufa_list *node = new_node(element, NULL, NULL, NULL);

	struct ufa_list *last;
	if (list != NULL) {
		last = ufa_list_get_last(list);
		last->next = node;
		node->prev = last;
	}
	return (list != NULL) ? list : node;
}

struct ufa_list *ufa_list_append2(struct ufa_list *list,
				  const void *element,
				  ufa_list_free_fn_t free_func)
{
	// FIXME remove duplicaitions
	struct ufa_list *node = new_node(element, NULL, NULL, free_func);

	struct ufa_list *last;
	if (list != NULL) {
		last = ufa_list_get_last(list);
		last->next = node;
		node->prev = last;
	}
	return (list != NULL) ? list : node;
}

struct ufa_list *ufa_list_prepend(struct ufa_list *list, const void *element)
{
	struct ufa_list *node = new_node(element, NULL, NULL, NULL);
	link_before(list, node);
	return node;
}

struct ufa_list *ufa_list_prepend2(struct ufa_list *list,
				   const void *element,
				   ufa_list_free_fn_t free_func)
{
	// FIXME remove duplicaitions
	struct ufa_list *node = new_node(element, NULL, NULL, free_func);
	link_before(list, node);
	return node;
}

struct ufa_list *ufa_list_insert(struct ufa_list *list,
				 unsigned int pos,
				 const void *element)
{
	if (pos == 0) {
		return ufa_list_prepend(list, element);
	}

	struct ufa_list *l = ufa_list_get(list, pos);
	if (l == NULL) { /* not found */
		return ufa_list_append(list, element);
	}

	struct ufa_list *node = new_node(element, NULL, NULL, NULL);
	link_before(l, node);

	return list;
}

struct ufa_list *ufa_list_concat(struct ufa_list *list, struct ufa_list *list2)
{
	if (list == NULL) {
		return list2;
	}

	if (list2 == NULL) {
		return list;
	}

	struct ufa_list *last = ufa_list_get_last(list);
	last->next = list2;
	list2->prev = last;

	return list;

}
struct ufa_list *ufa_list_get(struct ufa_list *list, unsigned int pos)
{
	for (unsigned int x = 0; (x < pos && list != NULL); x++) {
		list = list->next;
	}
	return list;
}

struct ufa_list *ufa_list_get_first(struct ufa_list *list)
{
	if (list == NULL) {
		return NULL;
	}
	UFA_LIST_REWIND(list);
	return list;
}

struct ufa_list *ufa_list_get_last(struct ufa_list *list)
{
	for (; (list->next != NULL); list = list->next)
		;
	return list;
}

unsigned int ufa_list_size(const struct ufa_list *list)
{
	unsigned int size = 0;
	for (size = 0; (list != NULL); list = list->next, size++)
		;
	return size;
}

void ufa_list_foreach(struct ufa_list *list, ufa_list_foreach_fn_t forfunc,
		      void *user_data)
{
	for (; (list != NULL); list = list->next) {
		forfunc(list->data, user_data);
	}
}

/* just disconnects the node */
struct ufa_list *ufa_list_unlink_node(struct ufa_list *list,
				      struct ufa_list *node)
{
	if (node == NULL) {
		return list;
	}
	struct ufa_list *prev = node->prev;
	struct ufa_list *next = node->next;

	if (prev != NULL) {
		prev->next = next;
	}
	if (next != NULL) {
		next->prev = prev;
	}
	/* disconect the node */
	node->next = NULL;
	node->prev = NULL;

	return (list == node) ? next : list;
}


struct ufa_list *ufa_list_find_by_data(const struct ufa_list *list,
				       const void *data,
				       ufa_list_equal_fn_t eq_func)
{
	struct ufa_list *l = (struct ufa_list *) list;
	for (; (l != NULL); l = l->next) {
		if (eq_func(l->data, data)) {
			return l;
		}
	}
	return NULL;
}

struct ufa_list *ufa_list_clone(struct ufa_list *list,
				ufa_list_cpydata_fn_t copyfunc,
				ufa_list_free_fn_t freefunc)
{
	if (list == NULL) {
		return NULL;
	}

	struct ufa_list *new_list = NULL;
	for (UFA_LIST_EACH(i, list)) {

		void *p = i->data;
		if (copyfunc != NULL) {
			p = copyfunc(p);
		}
		new_list = ufa_list_prepend2(new_list, p, freefunc);
	}
	return ufa_list_reverse(new_list);
}

struct ufa_list *ufa_list_reverse(struct ufa_list *list)
{
	struct ufa_list *next = list;
	struct ufa_list *last = list;
	while (next) {
		struct ufa_list *next_next = next->next;
		if (next->next == NULL) {
			last = next;
		}

		// swap elements
		struct ufa_list *aux = next->next;
		next->next = next->prev;
		next->prev = aux;

		next = next_next;
	}

	return last;
}

void ufa_list_free(struct ufa_list *list)
{
	// only free nodes
	while (list != NULL) {
		struct ufa_list *to_free = list;
		list = list->next;
		if (to_free->free_func) {
			to_free->free_func(to_free->data);
		}
		ufa_free(to_free);
	}
}

void ufa_list_free_full(struct ufa_list *list, ufa_list_free_fn_t freefunc)
{
	while (list != NULL) {
		if (list->free_func) {
			list->free_func(list->data);
		} else if (freefunc != NULL) {
			freefunc(list->data);
		}
		struct ufa_list *to_free = list;
		list = list->next;
		ufa_free(to_free);
	}
}

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static struct ufa_list *new_node(const void *data,
				 struct ufa_list *prev,
				 struct ufa_list *next,
				 ufa_list_free_fn_t free_func)
{
	struct ufa_list *node = ufa_malloc(sizeof(struct ufa_list));
	node->data = (void *) data;
	node->next = next;
	node->prev = prev;
	node->free_func = free_func;

	return node;
}

static void link_before(struct ufa_list *list, struct ufa_list *newnode)
{
	assert(newnode->next == NULL && newnode->prev == NULL);
	if (list != NULL) {
		newnode->next = list;
		newnode->prev = list->prev;
		list->prev = newnode;

		if (newnode->prev != NULL) {
			newnode->prev->next = newnode;
		}
	}
}
