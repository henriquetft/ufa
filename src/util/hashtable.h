/* ========================================================================== */
/* Copyright (c) 2022-2023 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for a simple hash table implementation                         */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <stdbool.h>

#define UFA_HASH_DEFAULT_ARRAY_SIZE    10
#define UFA_HASH_DEFAULT_LOAD_FACTOR   0.7f


typedef int (*ufa_hash_fn_t)(const void *data);
typedef bool (*ufa_hash_equal_fn_t)(const void *o1, const void *o2);
typedef void (*ufa_hash_free_fn_t)(void *data);
typedef int (*ufa_hash_foreach_fn_t)(void *k, void *v, void *user_data);

typedef struct ufa_hashtable ufa_hashtable_t;

#define UFA_HASHTABLE_STRING()                                                 \
	ufa_hashtable_new((ufa_hash_fn_t) ufa_str_hash,                        \
			  (ufa_hash_equal_fn_t) ufa_str_equals, ufa_free,      \
			  ufa_free)

ufa_hashtable_t *ufa_hashtable_new_full(ufa_hash_fn_t hashfunc,
					ufa_hash_equal_fn_t eqfunc,
					ufa_hash_free_fn_t freekey,
					ufa_hash_free_fn_t freevalue,
					int bucket_size,
					float loadfactor);

ufa_hashtable_t *ufa_hashtable_new(ufa_hash_fn_t hashfunc,
				   ufa_hash_equal_fn_t eqfunc,
				   ufa_hash_free_fn_t freekey,
				   ufa_hash_free_fn_t freevalue);

bool ufa_hashtable_put_full(ufa_hashtable_t *table, void *key, void *value,
			    ufa_hash_free_fn_t freek, ufa_hash_free_fn_t freev);

bool ufa_hashtable_put(ufa_hashtable_t *table, void *key, void *value);

void *ufa_hashtable_get(ufa_hashtable_t *table, const void *key);

int ufa_hashtable_has_key(ufa_hashtable_t *table, const void *key);

bool ufa_hashtable_remove(ufa_hashtable_t *table, const void *key);

int ufa_hashtable_size(ufa_hashtable_t *table);

void ufa_hashtable_foreach(ufa_hashtable_t *table, ufa_hash_foreach_fn_t func,
			   void *user_data);

void ufa_hashtable_clear(ufa_hashtable_t *table);

void ufa_hashtable_rehash(ufa_hashtable_t *table);

struct ufa_list *ufa_hashtable_keys(ufa_hashtable_t *table);

struct ufa_list *ufa_hashtable_values(ufa_hashtable_t *table);

void ufa_hashtable_free(ufa_hashtable_t *table);

#endif /* HASHTABLE_H_ */