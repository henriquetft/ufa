/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for hashtable.c                                                 */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/hashtable.h"
#include "util/list.h"
#include "util/misc.h"
#include "util/string.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>


/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define ASSERT_STR_IN_LIST(str, list)                                          \
	ck_assert(ufa_list_contains(                                           \
	    list, str, (ufa_list_equal_fn_t) ufa_str_equals))


/* ========================================================================== */
/* TEST FUNCTIONS                                                             */
/* ========================================================================== */


START_TEST(values_ok)
{
	ufa_hashtable_t *table = ufa_hashtable_new(
	    (ufa_hash_fn_t) ufa_str_hash,
	    (ufa_hash_equal_fn_t) ufa_str_equals,
	    ufa_free,
	    ufa_free);

	ufa_hashtable_put(table, ufa_str_dup("test1"), ufa_str_dup("value 1"));
	ufa_hashtable_put(table, ufa_str_dup("test2"), ufa_str_dup("value 2"));
	ufa_hashtable_put(table, ufa_str_dup("test3"), ufa_str_dup("value 3"));

	struct ufa_list *values = ufa_hashtable_values(table);
	ASSERT_STR_IN_LIST("value 1", values);
	ASSERT_STR_IN_LIST("value 2", values);
	ASSERT_STR_IN_LIST("value 3", values);
	ck_assert_int_eq(3, ufa_list_size(values));

	ufa_list_free(values);
	ufa_hashtable_free(table);
}
END_TEST


START_TEST(keys_ok)
{
	ufa_hashtable_t *table = ufa_hashtable_new(
	    (ufa_hash_fn_t) ufa_str_hash,
	    (ufa_hash_equal_fn_t) ufa_str_equals,
	    ufa_free,
	    ufa_free);

	ufa_hashtable_put(table, ufa_str_dup("test1"), ufa_str_dup("value 1"));
	ufa_hashtable_put(table, ufa_str_dup("test2"), ufa_str_dup("value 2"));
	ufa_hashtable_put(table, ufa_str_dup("test3"), ufa_str_dup("value 3"));

	struct ufa_list *values = ufa_hashtable_keys(table);
	ASSERT_STR_IN_LIST("test1", values);
	ASSERT_STR_IN_LIST("test2", values);
	ASSERT_STR_IN_LIST("test3", values);
	ck_assert_int_eq(3, ufa_list_size(values));

	ufa_list_free(values);
	ufa_hashtable_free(table);
}
END_TEST


/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */

Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("Hashtable");

	/* Core test case */
	tc_core = tcase_create("core");
	tcase_add_test(tc_core, values_ok);
	tcase_add_test(tc_core, keys_ok);

	/* Add test cases to suite */
	suite_add_tcase(s, tc_core);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = repo_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}