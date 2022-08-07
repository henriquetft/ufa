/*
 * Copyright (c) 2022 Henrique Teófilo
 * All rights reserved.
 *
 * A simple hash table implementation.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include "util/list.h"
#include "hashtable.h"
#include "logging.h"
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

/* Mapping function that unsets MSB (to prevent negatives) */
#define MAP(hashcode, size) ((hashcode & 0x7FFFFFFF) % size)

static struct node {
	void *key;
	void *value;
	struct node *next;
};

struct ufa_hashtable {
	struct node **buckets;  /* array */
	int bucket_size;	/* array size */
	int num_elements;       /* number of elements stored */
	ufa_hash_func_t func;   /* function to get hash code from keys */

	ufa_hash_equal_func_t eqfunc;
	ufa_hash_free_func_t freekeys;
	ufa_hash_free_func_t freevalues;

	float loadfactor;
	int threshold;
};

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

ufa_hashtable_t *ufa_hashtable_new_full(ufa_hash_func_t hfunc,
					ufa_hash_equal_func_t eqfunc,
					ufa_hash_free_func_t freek,
					ufa_hash_free_func_t freev,
					int bucket_size,
					float loadfactor)
{
	ufa_hashtable_t *table = malloc(sizeof *table);
	table->bucket_size = bucket_size;
	table->buckets = calloc(sizeof(struct node *), table->bucket_size);
	table->num_elements = 0;

	table->func = hfunc;
	table->eqfunc = eqfunc;
	table->freekeys = freek;
	table->freevalues = freev;
	table->loadfactor = loadfactor;

	table->threshold = (int)(table->bucket_size * table->loadfactor);

	return table;
}

ufa_hashtable_t *ufa_hashtable_new(ufa_hash_func_t hfunc,
				   ufa_hash_equal_func_t eqfunc,
				   ufa_hash_free_func_t freek,
				   ufa_hash_free_func_t freev)
{
	return ufa_hashtable_new_full(hfunc, eqfunc, freek, freev,
				      UFA_HASH_DEFAULT_ARRAY_SIZE,
				      UFA_HASH_DEFAULT_LOAD_FACTOR);
}

bool ufa_hashtable_put(ufa_hashtable_t *table, void *key, void *value)
{
	struct node *node = NULL;
	int map = -1;
	bool ret;
	/* check whether a key exists */
	find_node(table, key, &node, NULL, &map);

	if (node != NULL) {
		if (table->freevalues) {
			table->freevalues(node->value);
		}

		if (table->freekeys) {
			table->freekeys(node->key);
		}
		node->key = key;
		node->value = value;
		ret = false;
	} else {
		/* adding */
		assert(map >= 0 && map < table->bucket_size);
		node = malloc(sizeof *node);
		node->key = key;
		node->value = value;
		node->next = table->buckets[map];
		table->buckets[map] = node;

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

		if (table->freekeys) {
			table->freekeys(to_del->key);
		}
		if (table->freevalues) {
			table->freevalues(to_del->value);
		}
		free(to_del);
		table->num_elements--;
	}

	return found;
}

int ufa_hashtable_size(ufa_hashtable_t *table)
{
	return table->num_elements;
}

/* the data structure cannot be modified while iterating with this function */
void ufa_hashtable_foreach(ufa_hashtable_t *table, ufa_hash_foreach_func_t func,
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
			if (table->freekeys) {
				table->freekeys(to_del->key);
			}
			if (table->freevalues) {
				table->freevalues(to_del->value);
			}
			free(to_del);
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

	struct node **buckets = calloc(sizeof(struct node *), new_size);
	table->bucket_size = new_size;
	table->num_elements = 0;
	table->buckets = buckets;
	table->threshold = (int)(table->bucket_size * table->loadfactor);

	for (int x = 0; x < old_size; x++) {
		struct node *node = old_buckets[x];
		while (node != NULL) {
			/* TODO if put call rehash again ? */
			ufa_hashtable_put(table, node->key, node->value);
			struct node *old = node;
			node = node->next;
			free(old);
		}
	}
	free(old_buckets);
}

struct ufa_list *ufa_hashtable_keys(ufa_hashtable_t *table)
{
	struct ufa_list *keys = NULL;
	for (int x = 0; x < table->bucket_size; x++) {
		struct node *node = table->buckets[x];
		while (node != NULL) {
			keys = ufa_list_append(keys, node->key);
			node = node->next;
		}
	}
	return keys;
}



struct ufa_list *ufa_hashtable_values(ufa_hashtable_t *table)
{
	struct ufa_list *values = NULL;
	for (int x = 0; x < table->bucket_size; x++) {
		struct node *node = table->buckets[x];
		while (node != NULL) {
			values = ufa_list_append(values, node->value);
			node = node->next;
		}
	}
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