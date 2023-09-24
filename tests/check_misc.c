/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for misc.c                                                      */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#include "util/misc.h"
#include "util/string.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>


/* ========================================================================== */
/* TEST FUNCTIONS                                                             */
/* ========================================================================== */

START_TEST(resolvepath_nothing)
{
	char *resolved = ufa_util_resolvepath("/home/aaa/bbb");
	ck_assert_str_eq("/home/aaa/bbb", resolved);

	ufa_free(resolved);
}END_TEST


START_TEST(resolvepath_dot_nothing)
{
	char *resolved = ufa_util_resolvepath("/home/./aaa/bbb");
	ck_assert_str_eq("/home/aaa/bbb", resolved);

	ufa_free(resolved);
}
END_TEST


START_TEST(resolvepath_dotdot)
{
	char *resolved1 = ufa_util_resolvepath("/home/../aaa/bbb");
	ck_assert_str_eq("/aaa/bbb", resolved1);

	char *resolved2 =
	    ufa_util_resolvepath("/home/hello/./satoshi/../../../aaa/bbb");
	ck_assert_str_eq("/aaa/bbb", resolved2);

	char *resolved3 = ufa_util_resolvepath("/home/hello/../../../aaa/bbb");
	ck_assert_str_eq("/aaa/bbb", resolved3);

	ufa_free(resolved1);
	ufa_free(resolved2);
	ufa_free(resolved3);
}
END_TEST


START_TEST(resolvepath_dot)
{
	char *resolved = ufa_util_resolvepath(".");

	char *current = ufa_util_get_current_dir();

	ck_assert_str_eq(current, resolved);

	char *resolved2 = ufa_util_resolvepath("./oi");

	char *expected2 = ufa_str_concat(current, "/oi");
	ck_assert_str_eq(expected2, resolved2);

	ufa_free(resolved);
	ufa_free(resolved2);

}
END_TEST


START_TEST(resolvepath_current_dir)
{
	char *s = "/aaa/bbb";
	char *resolved = ufa_util_resolvepath(s+1);

	char *current = ufa_util_get_current_dir();

	char *expected = ufa_str_concat(current, s);
	ck_assert_str_eq(expected, resolved);

	ufa_free(resolved);
	ufa_free(current);
	ufa_free(expected);
}
END_TEST



/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */

Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("misc");

	/* Core test case */
	tc_core = tcase_create("core");
	tcase_add_test(tc_core, resolvepath_nothing);
	tcase_add_test(tc_core, resolvepath_dot_nothing);
	tcase_add_test(tc_core, resolvepath_current_dir);

	tcase_add_test(tc_core, resolvepath_dotdot);
	tcase_add_test(tc_core, resolvepath_dot);


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