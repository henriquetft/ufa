/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for string.c                                                      */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/list.h"
#include "util/string.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

/* ========================================================================== */
/* TEST FUNCTIONS                                                             */
/* ========================================================================== */

START_TEST(str_split_ok)
{
	const char *str = "hello_-_ufa_-_you_-_rock";
	struct ufa_list *head;
	struct ufa_list *list = head = ufa_str_split(str, "_-_");

	ck_assert_int_eq(4, ufa_list_size(list));

	ck_assert_str_eq("hello", list->data);
	list = list->next;
	ck_assert_str_eq("ufa", list->data);
	list = list->next;
	ck_assert_str_eq("you", list->data);
	list = list->next;
	ck_assert_str_eq("rock", list->data);

	ck_assert(list->next == NULL);

	ufa_list_free(head);
}
END_TEST



/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */

Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("string");

	/* Core test case */
	tc_core = tcase_create("core");
	tcase_add_test(tc_core, str_split_ok);

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