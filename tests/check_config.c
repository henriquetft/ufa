/* ========================================================================== */
/* Copyright (c) 2023-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for config.c                                                    */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/config.h"
#include "core/errors.h"
#include "util/hashtable.h"
#include "util/list.h"
#include "util/misc.h"
#include "util/string.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// FIXME  tcase_add_unchecked_fixture para o inicio e fim de um teste total

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define TMP_DIRNAME1 "test_dir1"
#define TMP_DIRNAME2 "test_dir2"

char TMP_CONFIG_DIR[]         = "/tmp/ufa-config-XXXXXX";
char TMP_REPO_PATH[]          = "/tmp/ufa-repo-XXXXXX";
char TMP_ERROR_REPO_PATH[]    = "/tmp/ufa-repo-error-XXXXXX";
char TMP_ERROR_REPO_NO_PERM[] = "/tmp/ufa-repo-write-error-XXXXXX";
char *TMP_DIR1 = NULL;
char *TMP_DIR2 = NULL;

#define ASSERT_STR_IN_LIST(str, list)                                          \
	ck_assert(ufa_list_contains(                                           \
	    list, str, (ufa_list_equal_fn_t) ufa_str_equals))


/* ========================================================================== */
/* FIXTURE FUNCTIONS                                                          */
/* ========================================================================== */

void setup_all_test(void)
{
	mkdtemp(TMP_CONFIG_DIR);
	mkdtemp(TMP_REPO_PATH);
	mkdtemp(TMP_ERROR_REPO_NO_PERM);
	int c = 0;
	if ((c = chmod(TMP_ERROR_REPO_NO_PERM, S_IRUSR|S_IRGRP|S_IROTH))) {
		fprintf(stderr, "Error on chmod: %d\n", c);
	}

	setenv("XDG_CONFIG_HOME", TMP_CONFIG_DIR, 1);

	TMP_DIR1 = ufa_str_sprintf("%s/%s", TMP_REPO_PATH, TMP_DIRNAME1);
	TMP_DIR2 = ufa_str_sprintf("%s/%s", TMP_REPO_PATH, TMP_DIRNAME2);

	struct ufa_error *error = NULL;
	ufa_util_mkdir(TMP_DIR1, &error);
	ufa_error_print(error);
	ufa_util_mkdir(TMP_DIR2, &error);
	ufa_error_print(error);

	const char *config_home = getenv("XDG_CONFIG_HOME");
	printf("CONFIG HOME.........: %s\n", config_home);
	printf("DIR 1...............: %s\n", TMP_DIR1);
	printf("DIR 2...............: %s\n", TMP_DIR2);
	printf("DIR NO PERM.........: %s\n", TMP_ERROR_REPO_NO_PERM);
}

void teardown_all_test(void)
{
	ufa_util_rmdir(TMP_CONFIG_DIR, NULL);

	ufa_util_rmdir(TMP_DIR1, NULL);
	ufa_util_rmdir(TMP_DIR2, NULL);
	ufa_util_rmdir(TMP_ERROR_REPO_NO_PERM, NULL);

	ufa_free(TMP_DIR1);
	ufa_free(TMP_DIR2);
}

/* ========================================================================== */
/* TEST FUNCTIONS                                                             */
/* ========================================================================== */

START_TEST(dirs_empty_ok)
{
	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_config_dirs(true, &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert_int_eq(0, ufa_list_size(list));
}
END_TEST

START_TEST(config_notdir_error)
{
	setenv("XDG_CONFIG_HOME", TMP_ERROR_REPO_PATH, 1);
	struct ufa_error *error = NULL;
	struct ufa_list *list  = ufa_config_dirs(true, &error);
	setenv("XDG_CONFIG_HOME", TMP_CONFIG_DIR, 1);

	ck_assert(error != NULL);
	ck_assert_int_eq(UFA_ERROR_NOTDIR, error->code);
	ufa_error_print_and_free(error);
	ck_assert_int_eq(0, ufa_list_size(list));
}
END_TEST


START_TEST(add_dir_error_notdir)
{
	struct ufa_error *error = NULL;
	bool ret = ufa_config_add_dir("./oi", &error);
	ufa_error_print(error);
	ck_assert(ret == false);
	ck_assert(error != NULL);
	ck_assert(error->code == UFA_ERROR_NOTDIR);

	ufa_error_free(error);
}
END_TEST


START_TEST(add_dirs_ok)
{
	struct ufa_error *error = NULL;
	bool ret1 = ufa_config_add_dir(TMP_DIR1, &error);
	ck_assert(ret1 == true);
	ck_assert(error == NULL);

	bool ret2 = ufa_config_add_dir(TMP_DIR2, &error);
	ck_assert(ret2 == true);
	ck_assert(error == NULL);
}
END_TEST


START_TEST(list_dirs_ok)
{
	ufa_config_add_dir(TMP_DIR1, NULL);
	ufa_config_add_dir(TMP_DIR2, NULL);

	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_config_dirs(true, &error);

	ufa_error_print(error);
	ck_assert(error == NULL);
	ASSERT_STR_IN_LIST(TMP_DIR1, list);
	ASSERT_STR_IN_LIST(TMP_DIR2, list);
	ck_assert_int_eq(2, ufa_list_size(list));

	ufa_list_free(list);
}
END_TEST

START_TEST(list_dirs_write_error)
{
	struct ufa_error *error = NULL;
	setenv("XDG_CONFIG_HOME", TMP_ERROR_REPO_NO_PERM, 1);
	struct ufa_list *list = ufa_config_dirs(true, &error);
	setenv("XDG_CONFIG_HOME", TMP_CONFIG_DIR, 1);

	ufa_error_print(error);
	ck_assert(list == NULL);
	ck_assert(error != NULL);
	ck_assert_int_eq(UFA_ERROR_FILE, error->code);
}
END_TEST




/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */

Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("Config");

	/* Core test case */
	tc_core = tcase_create("core");
	//tcase_add_checked_fixture(tc_core, setup_test, teardown_test);
	tcase_add_unchecked_fixture(tc_core, setup_all_test, teardown_all_test);
	tcase_add_test(tc_core, dirs_empty_ok);
	tcase_add_test(tc_core, list_dirs_write_error);
	tcase_add_test(tc_core, add_dir_error_notdir);
	tcase_add_test(tc_core, config_notdir_error);
	tcase_add_test(tc_core, add_dirs_ok);
	tcase_add_test(tc_core, list_dirs_ok);

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