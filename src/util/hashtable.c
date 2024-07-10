/* ========================================================================== */
/* Copyright (c) 2022-2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* A simple hash table (implementation of hashtable.h)                        */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/list.h"
#include "util/misc.h"
#include "hashtable.h"
#include "logging.h"
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define UFA_HASH_DEFAULT_ARRAY_SIZE    10
#define UFA_HASH_DEFAULT_LOAD_FACTOR   0.7f

/* Mapping function that unsets MSB (to prevent negatives) */
#define MAP(hashcode, size) ((hashcode & 0x7FFFFFFF) % size)

struct node {
	void *key;
	void *value;
	ufa_hash_free_fn_t freekey;
	ufa_hash_free_fn_t freevalue;
	struct node *next;
};

struct ufa_hashtable {
	struct node **buckets;  /* Array */
	int bucket_size;	/* Array size */
	int num_elements;       /* Number of elements stored */
	ufa_hash_fn_t func;     /* Function to get hash code from keys */

	ufa_hash_equal_fn_t eqfunc;
	ufa_hash_free_fn_t freekeys;
	ufa_hash_free_fn_t freevalues;

	float loadfactor;
	int threshold;
};


/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void find_node(ufa_hashtable_t *table, const void *key,
		      struct node **node, struct node **prev, int *bucket);

/* ========================================================================== */
/* FUNCTIONS FROM hashtable.h                                                 */
/* ========================================================================== */

ufa_hashtable_t *ufa_hashtable_new_full(ufa_hash_fn_t hfunc,
					ufa_hash_equal_fn_t eqfunc,
					ufa_hash_free_fn_t freek,
					ufa_hash_free_fn_t freev,
					int bucket_size,
					float loadfactor)
{
	ufa_hashtable_t *table = ufa_malloc(sizeof *table);
	table->bucket_size = bucket_size;
	table->buckets = ufa_calloc(sizeof(struct node *), table->bucket_size);
	table->num_elements = 0;

	table->func = hfunc;
	table->eqfunc = eqfunc;
	table->freekeys = freek;
	table->freevalues = freev;
	table->loadfactor = loadfactor;

	table->threshold = (int)(table->bucket_size * table->loadfactor);

	return table;
}

ufa_hashtable_t *ufa_hashtable_new(ufa_hash_fn_t hfunc,
				   ufa_hash_equal_fn_t eqfunc,
				   ufa_hash_free_fn_t freek,
				   ufa_hash_free_fn_t freev)
{
	return ufa_hashtable_new_full(hfunc, eqfunc, freek, freev,
				      UFA_HASH_DEFAULT_ARRAY_SIZE,
				      UFA_HASH_DEFAULT_LOAD_FACTOR);
}

bool ufa_hashtable_put(ufa_hashtable_t *table, void *key, void *value)
{
	return ufa_hashtable_put_full(table, key, value, table->freekeys,
				      table->freevalues);
}

bool ufa_hashtable_put_full(ufa_hashtable_t *table,
			    void *key,
			    void *value,
			    ufa_hash_free_fn_t freek, ufa_hash_free_fn_t freev)
{
	struct node *node = NULL;
	int map = -1;
	bool ret;
	/* check whether a key exists */
	find_node(table, key, &node, NULL, &map);
	if (node != NULL) {
		if (node->freevalue) {
			node->freevalue(node->value);
		}

		if (node->freekey) {
			node->freekey(node->key);
		}
		node->key = key;
		node->value = value;
		node->freekey = freek;
		node->freevalue = freev;
		ret = false;
	} else {
		/* adding */
		assert(map >= 0 && map < table->bucket_size);
		node = ufa_malloc(sizeof *node);
		node->key = key;
		node->value = value;
		node->next = table->buckets[map];
		table->buckets[map] = node;
		node->freekey = freek;
		node->freevalue = freev;

		table->num_elements++;

		if (table->num_elements > table->threshold) {
			ufa_debug(__FILE__ ": threshold reached. Rehashing...");
			ufa_hashtable_rehash(table);
		}
		ret = true;
	}
	return ret;
}

void *ufa_hashtable_get(ufa_hashtable_t *table, const void *key)
{
	struct node *node = NULL;
	find_node(table, key, &node, NULL, NULL);
	if (node != NULL) {
		return node->value;
	} else {
		return NULL;
	}
}

int ufa_hashtable_has_key(ufa_hashtable_t *table, const void *key)
{
	struct node *node = NULL;
	find_node(table, key, &node, NULL, NULL);
	return (node != NULL);
}

bool ufa_hashtable_remove(ufa_hashtable_t *table, const void *key)
{
	struct node *to_del = NULL;
	struct node *prev = NULL;
	int map = -1;

	find_node(table, key, &to_del, &prev, &map);

	bool found = (to_del != NULL);

	if (found) {
		if (prev == NULL) {
			table->buckets[map] = to_del->next;
		} else {
			prev->next = to_del->next;
		}

		if (to_del->freekey) {
			to_del->freekey(to_del->key);
		}
		if (to_del->freevalue) {
			to_del->freevalue(to_del->value);
		}
		ufa_free(to_del);
		table->num_elements--;
	}

	return found;
}

int ufa_hashtable_size(ufa_hashtable_t *table)
{
	return table->num_elements;
}

/* the data structure cannot be modified while iterating with this function */
void ufa_hashtable_foreach(ufa_hashtable_t *table, ufa_hash_foreach_fn_t func,
			   void *user_data)
{
	int x;
	for (x = 0; x < table->bucket_size; x++) {
		struct node *node = table->buckets[x];
		while (node != NULL) {
			func(node->key, node->value, user_data);
			node = node->next;
		}
	}
}

void ufa_hashtable_clear(ufa_hashtable_t *table)
{
	int x;
	for (x = 0; x < table->bucket_size; x++) {
		struct node *node = table->buckets[x];
		while (node != NULL) {
			struct node *to_del = node;
			node = node->next;
			if (to_del->freekey) {
				to_del->freekey(to_del->key);
			}
			if (to_del->freevalue) {
				to_del->freevalue(to_del->value);
			}
			ufa_free(to_del);
		}
		table->buckets[x] = NULL;
	}
	table->num_elements = 0;
}

void ufa_hashtable_rehash(ufa_hashtable_t *table)
{
	struct node **old_buckets = table->buckets;
	int old_size = table->bucket_size;
	int new_size = old_size * 2;

	struct node **buckets = ufa_calloc(sizeof(struct node *), new_size);
	table->bucket_size = new_size;
	table->num_elements = 0;
	table->buckets = buckets;
	table->threshold = (int)(table->bucket_size * table->loadfactor);

	for (int x = 0; x < old_size; x++) {
		struct node *node = old_buckets[x];
		while (node != NULL) {
			/* TODO if put call rehash again ? */
			ufa_hashtable_put_full(table, node->key, node->value,
					       node->freekey, node->freevalue);
			struct node *old = node;
			node = node->next;
			ufa_free(old);
		}
	}
	ufa_free(old_buckets);
}

struct ufa_list *ufa_hashtable_keys(ufa_hashtable_t *table)
{
	struct ufa_list *keys = NULL;
	for (int x = 0; x < table->bucket_size; x++) {
		struct node *node = table->buckets[x];
		while (node != NULL) {
			keys = ufa_list_prepend(keys, node->key);
			node = node->next;
		}
	}
	keys = ufa_list_reverse(keys);
	return keys;
}



struct ufa_list *ufa_hashtable_values(ufa_hashtable_t *table)
{
	struct ufa_list *values = NULL;
	for (int x = 0; x < table->bucket_size; x++) {
		struct node *node = table->buckets[x];
		while (node != NULL) {
			values = ufa_list_prepend(values, node->value);
			node = node->next;
		}
	}
	values = ufa_list_reverse(values);
	return values;
}


void ufa_hashtable_free(ufa_hashtable_t *table)
{
	if (table != NULL) {
		ufa_hashtable_clear(table);
		free(table->buckets);
		free(table);
	}
}


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static void find_node(ufa_hashtable_t *table, const void *key,
		      struct node **node, struct node **prev, int *bucket)
{
	int h = table->func(key);
	int map = MAP(h, table->bucket_size);
	struct node *n = table->buckets[map];

	struct node *iter = NULL;
	struct node *p = NULL;
	for (; n != NULL; n = n->next) {
		if (table->eqfunc(key, n->key)) {
			iter = n;
			break;
		}
		p = n;
	}

	if (bucket != NULL) {
		*bucket = map;
	}

	if (node != NULL) {
		*node = iter;
	}

	if (prev != NULL) {
		*prev = p;
	}
}