/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of a doubly linked list.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"

// =================================================================================================
// AUXILIARY FUNCTIONS
// =================================================================================================

static void
link_before(ufa_list_t *list, ufa_list_t *newnode);

static ufa_list_t *
new_node(void *data, ufa_list_t *prev, ufa_list_t *next);


// =================================================================================================
// FUNCTIONS FROM list.h
// =================================================================================================

/**
 * Adds the element after the last one.
 * Returns: the same list with the new element in the tail, or the new node
 * (new single-element list) if list was NULL
 */
ufa_list_t *
ufa_list_append(ufa_list_t *list, void *element)
{
    ufa_list_t *node = new_node(element, NULL, NULL);

    ufa_list_t *last;
    if (list != NULL) {
        last = ufa_list_get_last(list);
        last->next = node;
        node->prev = last;
    }
    return (list != NULL) ? list : node;
}


ufa_list_t *
ufa_list_prepend(ufa_list_t *list, void *element)
{
    ufa_list_t *node = new_node(element, NULL, NULL);
    link_before(list, node);
    return node;
}


ufa_list_t *
ufa_list_insert(ufa_list_t *list, unsigned int pos, void *element)
{
    if (pos == 0) {
        return ufa_list_prepend(list, element);
    }

    ufa_list_t *l = ufa_list_get(list, pos);
    if (l == NULL) { /* not found */
        return ufa_list_append(list, element);
    }

    ufa_list_t *node = new_node(element, NULL, NULL);
    link_before(l, node);

    return list;
}


ufa_list_t *
ufa_list_get(ufa_list_t *list, unsigned int pos)
{
    unsigned int x;
    for (x = 0; (x < pos && list != NULL); x++) {
        list = list->next;
    }
    return list;
}


ufa_list_t *
ufa_list_get_first(ufa_list_t *list)
{
    if (list == NULL) {
        return NULL;
    }
    UFA_LIST_REWIND(list);
    return list;
}


ufa_list_t *
ufa_list_get_last(ufa_list_t *list)
{
    for (; (list->next != NULL); list = list->next)
        ;
    return list;
}

unsigned int
ufa_list_size(ufa_list_t *list)
{
    unsigned int size;
    for (size = 0; (list != NULL); list = list->next, size++)
        ;
    return size;
}


void
ufa_list_foreach(ufa_list_t *list, ufa_list_for_each_func forfunc, void *user_data)
{
    for (; (list != NULL); list = list->next) {
        forfunc(list->data, user_data);
    }
}


/* just disconnects the node */
ufa_list_t *
ufa_list_unlink_node(ufa_list_t *list, ufa_list_t *node)
{
    if (node == NULL) {
        return list;
    }
    ufa_list_t *prev = node->prev;
    ufa_list_t *next = node->next;

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


void
ufa_list_free_full(ufa_list_t *list, ufa_list_free_func freefunc)
{
    while (list != NULL) {
        if (freefunc != NULL) {
            freefunc(list->data);
        }
        ufa_list_t *toFree = list;
        list = list->next;
        free(toFree);
    }
}


void
ufa_list_free(ufa_list_t *list)
{
    ufa_list_free_full(list, NULL);
}

/* ==============   auxiliary (private) functions   ========================== */

static ufa_list_t *
new_node(void *data, ufa_list_t *prev, ufa_list_t *next)
{
    ufa_list_t *node = malloc(sizeof(ufa_list_t));
    node->data = data;
    node->next = next;
    node->prev = prev;

    return node;
}


static void
link_before(ufa_list_t *list, ufa_list_t *newnode)
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
