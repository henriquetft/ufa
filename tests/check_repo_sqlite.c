/* ========================================================================== */
/* Copyright (c) 2023-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for repo_sqlite.c                                               */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/repo.h"
#include "util/error.h"
#include "util/misc.h"
#include "util/string.h"
#include "core/errors.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static ufa_repo_t *global_repo = NULL;

char TMP_REPO_DIR[] = "/tmp/ufa-test-XXXXXX";
char *TMP_REPO_FILE = NULL;
char *TMP_UFAREPOFILE = NULL;
char *TMP_TEST_FILE1 = NULL;


const char *TAG1 = "tag1";
const char *TAG2 = "tag2";
const char *TAG3 = "tag3";

#define ASSERT_STR_IN_LIST(str, list)                                          \
	ck_assert(ufa_list_contains(                                           \
	    list, str, (ufa_list_equal_fn_t) ufa_str_equals))

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static void create_file(const char *file)
{
	int fd = open(file, O_RDWR | O_CREAT, 0600);
	if (fd != -1) {
		close(fd);
	}
}

static void init_files_repo_tmp()
{
	mkdtemp(TMP_REPO_DIR);

	TMP_REPO_FILE   = ufa_util_joinpath(TMP_REPO_DIR, "repo.sqlite", NULL);
	TMP_UFAREPOFILE = ufa_util_joinpath(TMP_REPO_DIR, ".ufarepo", NULL);
	TMP_TEST_FILE1  = ufa_util_joinpath(TMP_REPO_DIR, "testfile1", NULL);

	create_file(TMP_TEST_FILE1);

	printf("Repo dir.........: %s\n", TMP_REPO_DIR);
	printf("Test file 1......: %s\n", TMP_TEST_FILE1);
}

static void remove_files_repo_tmp()
{
	ufa_util_remove_file(TMP_REPO_FILE, NULL);
	ufa_util_remove_file(TMP_UFAREPOFILE, NULL);
	ufa_util_remove_file(TMP_TEST_FILE1, NULL);
	ufa_util_rmdir(TMP_REPO_DIR, NULL);

	ufa_free(TMP_REPO_FILE);
	ufa_free(TMP_UFAREPOFILE);
	ufa_free(TMP_TEST_FILE1);
}

static void insert_test_tags()
{
	ufa_repo_inserttag(global_repo, TAG1, NULL);
	ufa_repo_inserttag(global_repo, TAG2, NULL);
	ufa_repo_inserttag(global_repo, TAG3, NULL);
}


/* ========================================================================== */
/* FIXTURE FUNCTIONS                                                          */
/* ========================================================================== */

void setup_repo(void)
{
	struct ufa_error *error = NULL;
	init_files_repo_tmp();
	global_repo = ufa_repo_init(TMP_REPO_DIR, &error);
}

void teardown_repo(void)
{
	ufa_repo_free(global_repo);
	global_repo = NULL;
	remove_files_repo_tmp();
}


/* ========================================================================== */
/* TEST FUNCTIONS FOR ufa_repo_init                                           */
/* ========================================================================== */


START_TEST(init_error_notdir)
{
	struct ufa_error *error = NULL;
	ufa_repo_t *repo = ufa_repo_init("/home/sdoijasiofj/sdfoidsft", &error);
	//printf("%d %s\n", error->code, error->message);
	ck_assert(repo == NULL);
	ck_assert(error != NULL);
	ck_assert(error->code == UFA_ERROR_NOTDIR);

	ufa_error_print_and_free(error);

}
END_TEST

START_TEST(init_error_create_db)
{
	struct ufa_error *error = NULL;
	// FIXME assert not permission first
	ufa_repo_t *repo = ufa_repo_init("/root", &error);
	//printf("%d %s\n", error->code, error->message);
	ck_assert(repo == NULL);
	ck_assert(error != NULL);
	ck_assert(error->code == UFA_ERROR_DATABASE);

	ufa_error_print_and_free(error);

}
END_TEST


START_TEST(init_ok)
{
	struct ufa_error *error = NULL;
	init_files_repo_tmp();
	ufa_repo_t *repo = ufa_repo_init(TMP_REPO_DIR, &error);
	ck_assert(repo != NULL);
	ck_assert(ufa_util_isfile(TMP_REPO_FILE));
	ck_assert(ufa_util_isfile(TMP_UFAREPOFILE));

	// clean
	ufa_repo_free(repo);
	remove_files_repo_tmp();

	ck_assert(!ufa_util_isdir(TMP_REPO_DIR));
	ck_assert(!ufa_util_isfile(TMP_REPO_FILE));
	ck_assert(!ufa_util_isfile(TMP_UFAREPOFILE));
}
END_TEST

/* ========================================================================== */
/* TEST FUNCTIONS FOR tag management                                          */
/* ========================================================================== */

START_TEST(listtags_empty)
{
	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_repo_listtags(global_repo, &error);
	ck_assert(list == NULL && error == NULL);
}
END_TEST

START_TEST(listtags_ok)
{
	insert_test_tags();

	struct ufa_error *error = NULL;

	struct ufa_list *list = ufa_repo_listtags(global_repo, &error);
	ck_assert(list != NULL && error == NULL);

	struct ufa_list *tag1 = ufa_list_find_by_data(
	    list, TAG1, (ufa_list_equal_fn_t) ufa_str_equals);

	struct ufa_list *tag2 = ufa_list_find_by_data(
	    list, TAG2, (ufa_list_equal_fn_t) ufa_str_equals);

	struct ufa_list *tag3 = ufa_list_find_by_data(
	    list, TAG3, (ufa_list_equal_fn_t) ufa_str_equals);

	ck_assert(tag1 != NULL);
	ck_assert(tag2 != NULL);
	ck_assert(tag3 != NULL);
	ck_assert(tag1 != tag2 && tag2 != tag3 && tag1 != tag3);

	ufa_list_free(list);
}
END_TEST


START_TEST(listfiles_ok)
{
	insert_test_tags();

	struct ufa_error *error = NULL;

	ufa_repo_settag(global_repo, TMP_TEST_FILE1, TAG1, &error);
	ufa_error_print_and_free(error);
	error = NULL;

	ufa_repo_settag(global_repo, TMP_TEST_FILE1, TAG2, &error);
	ufa_error_print_and_free(error);
	error = NULL;

	ufa_repo_settag(global_repo, TMP_TEST_FILE1, TAG3, &error);
	ufa_error_print_and_free(error);
	error = NULL;

	char *filename = ufa_util_getfilename(TMP_TEST_FILE1);

	// Testing "/"
	struct ufa_list *list_root = ufa_repo_listfiles(global_repo,
							"/",
							&error);
	ASSERT_STR_IN_LIST(TAG1, list_root);
	ASSERT_STR_IN_LIST(TAG2, list_root);
	ASSERT_STR_IN_LIST(TAG3, list_root);
	ASSERT_STR_IN_LIST(".ufarepo", list_root);
	ck_assert_int_eq(4, ufa_list_size(list_root));

	// Testing "/TAG1"
	char *path_tag1 = ufa_str_sprintf("/%s", TAG1);
	struct ufa_list *list1 = ufa_repo_listfiles(global_repo,
						    path_tag1,
						    &error);
	ASSERT_STR_IN_LIST(TAG2, list1);
	ASSERT_STR_IN_LIST(TAG3, list1);
	ASSERT_STR_IN_LIST(".ufarepo", list1);
	ASSERT_STR_IN_LIST(filename, list1);
	ck_assert_int_eq(4, ufa_list_size(list1));

	// Testing "/TAG1/TAG2"
	char *path_tag1_2 = ufa_str_sprintf("/%s/%s", TAG1, TAG2);
	struct ufa_list *list2 = ufa_repo_listfiles(global_repo,
						    path_tag1_2,
						    &error);
	ASSERT_STR_IN_LIST(TAG3, list2);
	ASSERT_STR_IN_LIST(".ufarepo", list2);
	ASSERT_STR_IN_LIST(filename, list2);
	ck_assert_int_eq(3, ufa_list_size(list2));

	// Testing "/TAG1/TAG2/TAG3"
	char *path_tag1_2_3 = ufa_str_sprintf("/%s/%s/%s", TAG1, TAG2, TAG3);
	struct ufa_list *list3 = ufa_repo_listfiles(global_repo,
						    path_tag1_2_3,
						    &error);
	ASSERT_STR_IN_LIST(".ufarepo", list3);
	ASSERT_STR_IN_LIST(filename, list3);
	ck_assert_int_eq(2, ufa_list_size(list3));

	ufa_free(filename);
	ufa_list_free(list_root);
	ufa_list_free(list2);
	ufa_list_free(list3);
}
END_TEST

START_TEST(settag_ok)
{
	struct ufa_error *error = NULL;
	bool ret = ufa_repo_settag(global_repo, TMP_TEST_FILE1, TAG1, &error);
	ck_assert_msg(error == NULL, "%s", error->message);
	ck_assert(ret);

	struct ufa_list *list = NULL;
	ret = ufa_repo_gettags(global_repo, TMP_TEST_FILE1, &list, &error);
	ck_assert_msg(error == NULL, "%s", error->message);
	ck_assert(ret);
	ck_assert(list != NULL);
	ck_assert(ufa_list_size(list) == 1);
	ck_assert(ufa_str_equals(list->data, TAG1));
	ufa_list_free(list);
}
END_TEST


/* ========================================================================== */
/* TEST FUNCTIONS FOR ufa_repo_getrepopath                                    */
/* ========================================================================== */

START_TEST(getrepopath_ok)
{
	char *abs_tmp_repo_dir = ufa_util_abspath(TMP_REPO_DIR);
	char *repo_path = ufa_repo_getrepopath(global_repo);
	ck_assert_msg(ufa_str_equals(repo_path, abs_tmp_repo_dir),
		      "%s != %s",
		      repo_path,
		      TMP_REPO_DIR);
	ufa_free(repo_path);
	ufa_free(abs_tmp_repo_dir);
}
END_TEST

START_TEST(getrepopath_null)
{
     char *repo_path = ufa_repo_getrepopath(NULL);
     ck_assert(repo_path == NULL);
}
END_TEST


/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */

Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_init;
	TCase *tc_tag;
	TCase *tc_getrepopath;

	s = suite_create("Repo");

	/* Core test case */
	tc_init = tcase_create("init");
	tcase_add_test(tc_init, init_error_notdir);
	tcase_add_test(tc_init, init_error_create_db);
	tcase_add_test(tc_init, init_ok);

	/* Tag management case */
	tc_tag = tcase_create("tags");
	tcase_add_checked_fixture(tc_tag, setup_repo, teardown_repo);
	tcase_add_test(tc_tag, listtags_ok);
	tcase_add_test(tc_tag, listtags_empty);
	tcase_add_test(tc_tag, settag_ok);
	tcase_add_test(tc_tag, listfiles_ok);

	// FIXME test gettags

	/* Tag management case */
	tc_getrepopath = tcase_create("getrepopath");
	tcase_add_checked_fixture(tc_getrepopath, setup_repo, teardown_repo);
	tcase_add_test(tc_getrepopath, getrepopath_ok);
	tcase_add_test(tc_getrepopath, getrepopath_null);

	/* Add test cases to suite */
	suite_add_tcase(s, tc_init);
	suite_add_tcase(s, tc_tag);
	suite_add_tcase(s, tc_getrepopath);

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