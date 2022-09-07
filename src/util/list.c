/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of a doubly linked list.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include "list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void link_before(struct ufa_list *list, struct ufa_list *newnode);

static struct ufa_list *new_node(void *data, struct ufa_list *prev,
				 struct ufa_list *next);

/* ========================================================================== */
/* FUNCTIONS FROM list.h                                                      */
/* ========================================================================== */

/**
 * Adds the element after the last one.
 * Returns: the same list with the new element in the tail, or the new node
 * (new single-element list) if list was NULL
 */
struct ufa_list *ufa_list_append(struct ufa_list *list, void *element)
{
	struct ufa_list *node = new_node(element, NULL, NULL);

	struct ufa_list *last;
	if (list != NULL) {
		last = ufa_list_get_last(list);
		last->next = node;
		node->prev = last;
	}
	return (list != NULL) ? list : node;
}

struct ufa_list *ufa_list_prepend(struct ufa_list *list, void *element)
{
	struct ufa_list *node = new_node(element, NULL, NULL);
	link_before(list, node);
	return node;
}

struct ufa_list *ufa_list_insert(struct ufa_list *list, unsigned int pos,
				 void *element)
{
	if (pos == 0) {
		return ufa_list_prepend(list, element);
	}

	struct ufa_list *l = ufa_list_get(list, pos);
	if (l == NULL) { /* not found */
		return ufa_list_append(list, element);
	}

	struct ufa_list *node = new_node(element, NULL, NULL);
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
	unsigned int x;
	for (x = 0; (x < pos && list != NULL); x++) {
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

unsigned int ufa_list_size(struct ufa_list *list)
{
	unsigned int size;
	for (size = 0; (list != NULL); list = list->next, size++)
		;
	return size;
}

void ufa_list_foreach(struct ufa_list *list, ufa_list_for_each_func forfunc,
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

void ufa_list_free_full(struct ufa_list *list, ufa_list_free_func freefunc)
{
	while (list != NULL) {
		if (freefunc != NULL) {
			freefunc(list->data);
		}
		struct ufa_list *toFree = list;
		list = list->next;
		free(toFree);
	}
}

void ufa_list_free(struct ufa_list *list)
{
	ufa_list_free_full(list, NULL);
}

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static struct ufa_list *new_node(void *data, struct ufa_list *prev,
				 struct ufa_list *next)
{
	struct ufa_list *node = malloc(sizeof(struct ufa_list));
	node->data = data;
	node->next = next;
	node->prev = prev;

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
