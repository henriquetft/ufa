/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
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

const char *TMP_REPO_DIR = "./tmp";
const char *TMP_REPO_FILE = "./tmp/repo.sqlite";
const char *TMP_UFAREPOFILE = "./tmp/.ufarepo";
const char *TMP_TEST_FILE = "./tmp/test_file";

const char *TAG1 = "tag1";
const char *TAG2 = "tag2";
const char *TAG3 = "tag3";

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static void init_files_repo_tmp()
{
	ufa_util_mkdir(TMP_REPO_DIR, NULL);

}

static void remove_files_repo_tmp()
{
	ufa_util_remove_file(TMP_REPO_FILE, NULL);
	ufa_util_remove_file(TMP_UFAREPOFILE, NULL);
	ufa_util_remove_file(TMP_TEST_FILE, NULL);
	ufa_util_rmdir(TMP_REPO_DIR, NULL);
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

}
END_TEST

START_TEST(init_error_create_db)
{
	struct ufa_error *error = NULL;
	// FIXME assert not permission first
	ufa_repo_t *repo = ufa_repo_init("/root", &error);
	//printf("%d %s\n", error->code, error->message);
	ck_assert(repo == NULL);
	ck_assert(error->code == UFA_ERROR_DATABASE);

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
	    list, TAG1, (bool (*)(void *, void *))ufa_str_equals);

	struct ufa_list *tag2 = ufa_list_find_by_data(
	    list, TAG2, (bool (*)(void *, void *))ufa_str_equals);

	struct ufa_list *tag3 = ufa_list_find_by_data(
	    list, TAG3, (bool (*)(void *, void *))ufa_str_equals);

	ck_assert(tag1 != NULL);
	ck_assert(tag2 != NULL);
	ck_assert(tag3 != NULL);
	ck_assert(tag1 != tag2 && tag2 != tag3 && tag1 != tag3);
}
END_TEST


START_TEST(settag_ok)
{
	// we must create the file first
	const char *filepath = TMP_TEST_FILE;
	int fd = open(TMP_TEST_FILE, O_RDWR | O_CREAT);
	ck_assert(fd != -1);
	close(fd);

	struct ufa_error *error = NULL;
	bool ret = ufa_repo_settag(global_repo, filepath, TAG1, &error);
	ck_assert_msg(error == NULL, "%s", error->message);
	ck_assert(ret);

	struct ufa_list *list = NULL;
	ret = ufa_repo_gettags(global_repo, filepath, &list, &error);
	ck_assert_msg(error == NULL, "%s", error->message);
	ck_assert(ret);
	ck_assert(list != NULL);
	ck_assert(ufa_list_size(list) == 1);
	ck_assert(ufa_str_equals(list->data, TAG1));
}
END_TEST


/* ========================================================================== */
/* TEST FUNCTIONS FOR ufa_repo_getrepopath                                    */
/* ========================================================================== */

START_TEST(getrepopath_ok)
{
	char *abs_tmp_repo_dir = ufa_util_abspath(TMP_REPO_DIR);
	char *repo_path = ufa_repo_getrepopath(global_repo);
	ck_assert_msg(ufa_str_equals(repo_path, abs_tmp_repo_dir), "%s != %s",
		      repo_path, TMP_REPO_DIR);
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