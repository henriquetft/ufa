/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for list.c                                                      */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/list.h"
#include "util/misc.h"
#include "util/string.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

/* ========================================================================== */
/* TEST FUNCTIONS                                                             */
/* ========================================================================== */


START_TEST(append_ok)
{
	struct ufa_list *list = NULL;
	list = ufa_list_append(list, "aaa");
	ck_assert_str_eq("aaa", list->data);

	list = ufa_list_append(list, "bbb");
	ck_assert_str_eq("aaa", list->data);
	ck_assert_str_eq("bbb", list->next->data);

	ufa_list_free(list);
}
END_TEST


START_TEST(prepend_ok)
{
	struct ufa_list *list = NULL;
	list = ufa_list_prepend(list, "aaa");
	ck_assert_str_eq("aaa", list->data);

	list = ufa_list_prepend(list, "bbb");
	ck_assert_str_eq("bbb", list->data);
	ck_assert_str_eq("aaa", list->next->data);

	ufa_list_free(list);
}
END_TEST


START_TEST(reverse_ok)
{
	struct ufa_list *single = ufa_list_append(NULL, "a");
	ck_assert(single == ufa_list_reverse(single));

	struct ufa_list *list = NULL;
	list = ufa_list_append(list, "aaa");
	list = ufa_list_append(list, "bbb");
	list = ufa_list_append(list, "ccc");

	struct ufa_list *reversed = ufa_list_reverse(list);
	ck_assert_str_eq("ccc", reversed->data);
	ck_assert_str_eq("bbb", reversed->next->data);
	ck_assert_str_eq("aaa", reversed->next->next->data);

	reversed = ufa_list_reverse(reversed);
	ck_assert_str_eq("aaa", reversed->data);
	ck_assert_str_eq("bbb", reversed->next->data);
	ck_assert_str_eq("ccc", reversed->next->next->data);

	ufa_list_free(single);
	ufa_list_free(reversed);
}
END_TEST


START_TEST(prepend_reverse_ok)
{
	struct ufa_list *list = NULL;
	list = ufa_list_prepend(list, "aaa");
	list = ufa_list_prepend(list, "bbb");
	list = ufa_list_prepend(list, "ccc");
	list = ufa_list_reverse(list);

	ck_assert_str_eq("aaa", list->data);
	ck_assert_str_eq("bbb", list->next->data);
	ck_assert_str_eq("ccc", list->next->next->data);

	ufa_list_free(list);
}
END_TEST


START_TEST(clone_ok)
{
	struct ufa_list *list = NULL;

	list = ufa_list_prepend(list, "aaa");
	list = ufa_list_prepend(list, "bbb");
	list = ufa_list_prepend(list, "ccc");

	struct ufa_list *copy = ufa_list_clone(
	    list, (ufa_list_cpydata_fn_t) ufa_str_dup, ufa_free);

	ck_assert_int_eq(ufa_list_size(list), ufa_list_size(copy));

	struct ufa_list *head_list = list;
	struct ufa_list *head_copy = copy;

	int size = ufa_list_size(list);
	for (int x = 0; x < size; x++) {
		char *element1 = list->data;
		char *element2 = copy->data;
		ck_assert_str_eq(element1, element2);
		list = list->next;
		copy = copy->next;
	}

	ufa_list_free(head_list);
	ufa_list_free(head_copy);
}
END_TEST

START_TEST(get_ok)
{
	struct ufa_list *list = NULL;

	list = ufa_list_append(list, "aaa");
	list = ufa_list_append(list, "bbb");
	list = ufa_list_append(list, "ccc");

	ck_assert_str_eq("aaa", ufa_list_get(list, 0)->data);
	ck_assert_str_eq("bbb", ufa_list_get(list, 1)->data);
	ck_assert_str_eq("ccc", ufa_list_get(list, 2)->data);

	ufa_list_free(list);
}
END_TEST


/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */


Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_core;


	s = suite_create("list");

	/* Core test case */
	tc_core = tcase_create("core");
	tcase_add_test(tc_core, append_ok);
	tcase_add_test(tc_core, prepend_ok);
	tcase_add_test(tc_core, reverse_ok);
	tcase_add_test(tc_core, prepend_reverse_ok);
	tcase_add_test(tc_core, clone_ok);
	tcase_add_test(tc_core, get_ok);

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