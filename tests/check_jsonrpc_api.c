/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for JSON RPC API                                                */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "core/repo.h"
#include "util/error.h"
#include "util/misc.h"
#include "util/string.h"
#include "json/jsonrpc_api.h"
#include "json/jsonrpc_parser.h"
#include "json/jsonrpc_server.h"
#include <check.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define TAG1  "tag1"
#define TAG2  "tag2"
#define TAG3  "tag3"

static pthread_t thread_server;

static ufa_jsonrpc_api_t *api       = NULL;
static ufa_repo_t *global_repo      = NULL;
static ufa_jsonrpc_server_t *server = NULL;

char TMP_REPO_DIR[]          = "/tmp/ufa-test-XXXXXX";
char *TMP_REPO_FILE          = NULL;
char *TMP_UFAREPOFILE        = NULL;
char *TMP_TEST_FILE1         = NULL;
char *TMP_TEST_FILE2         = NULL;
char *TMP_TEST_FILE3         = NULL;
char *TMP_TEST_FILE_NOTFOUND = NULL;


char *TAGS[3] = {TAG1, TAG2, TAG3};


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

	TMP_REPO_FILE = ufa_util_joinpath(TMP_REPO_DIR, "repo.sqlite", NULL);
	TMP_UFAREPOFILE = ufa_util_joinpath(TMP_REPO_DIR, ".ufarepo", NULL);
	TMP_TEST_FILE1 = ufa_util_joinpath(TMP_REPO_DIR, "testfile1", NULL);
	TMP_TEST_FILE2 = ufa_util_joinpath(TMP_REPO_DIR, "testfile2", NULL);
	TMP_TEST_FILE3 = ufa_util_joinpath(TMP_REPO_DIR, "testfile3", NULL);
	TMP_TEST_FILE_NOTFOUND = ufa_util_joinpath(TMP_REPO_DIR, "n", NULL);

	create_file(TMP_TEST_FILE1);
	create_file(TMP_TEST_FILE2);
	create_file(TMP_TEST_FILE3);

	printf("Repo dir.........: %s\n", TMP_REPO_DIR);
	printf("Test file 1......: %s\n", TMP_TEST_FILE1);
	printf("Test file 2......: %s\n", TMP_TEST_FILE2);
	printf("Test file 3......: %s\n", TMP_TEST_FILE3);
}

static void remove_files_repo_tmp()
{
	ufa_util_remove_file(TMP_REPO_FILE, NULL);
	ufa_util_remove_file(TMP_UFAREPOFILE, NULL);
	ufa_util_remove_file(TMP_TEST_FILE1, NULL);
	ufa_util_remove_file(TMP_TEST_FILE2, NULL);
	ufa_util_remove_file(TMP_TEST_FILE3, NULL);
	ufa_util_rmdir(TMP_REPO_DIR, NULL);

	ufa_free(TMP_REPO_FILE);
	ufa_free(TMP_UFAREPOFILE);
	ufa_free(TMP_TEST_FILE1);
	ufa_free(TMP_TEST_FILE2);
	ufa_free(TMP_TEST_FILE3);
	ufa_free(TMP_TEST_FILE_NOTFOUND);
}

static void insert_test_tags()
{
	ufa_repo_inserttag(global_repo, TAG1, NULL);
	ufa_repo_inserttag(global_repo, TAG2, NULL);
	ufa_repo_inserttag(global_repo, TAG3, NULL);
}

static void corrupt_db()
{
	int fd = open(TMP_REPO_FILE, O_WRONLY);
	if (fd != -1) {
		write(fd, "a", 1);
		close(fd);
	}
}

/* ========================================================================== */
/* FIXTURE FUNCTIONS                                                          */
/* ========================================================================== */

static void *start_server(void *thread_data)
{
	printf("Starting jsonrpc server ...\n");
	struct ufa_error *error = NULL;
	server = ufa_jsonrpc_server_new();
	ufa_jsonrpc_server_start(server, &error);
	if (!server || error) {
		fprintf(stderr, "Error starting jsonrpc server\n");
		ufa_error_print_and_free(error);
		exit(EXIT_FAILURE);
	}
	return NULL;
}

void setup_repo(void)
{
	struct ufa_error *error = NULL;
	init_files_repo_tmp();
	global_repo = ufa_repo_init(TMP_REPO_DIR, &error);

	int ret = pthread_create(&thread_server, NULL, start_server, NULL);
	if (ret != 0) {
		fprintf(stderr, "Error creating thread!\n");
		exit(EXIT_FAILURE);
	}
	usleep(150 * 1000); // 150 ms
	api = ufa_jsonrpc_api_init(NULL);
}

void teardown_repo(void)
{
	ufa_repo_free(global_repo);
	global_repo = NULL;

	struct ufa_error *error = NULL;
	ufa_jsonrpc_server_stop(server, &error);
	ufa_error_print_and_free(error);

	ufa_jsonrpc_server_free(server);

	// Awaiting thread_server to finish
	pthread_join(thread_server, NULL);
	server = NULL;
	remove_files_repo_tmp();

	ufa_jsonrpc_api_close(api, NULL);
}

/* ========================================================================== */
/* TEST FUNCTIONS FOR TAG MANAGEMENT                                          */
/* ========================================================================== */

START_TEST(api_settag)
{
	bool c = ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG1, NULL);
	ck_assert(c);
}
END_TEST

START_TEST(api_settag_nonexistent_file)
{
	struct ufa_error *error = NULL;
	bool c = ufa_jsonrpc_api_settag(api, TMP_TEST_FILE_NOTFOUND,
					TAG1,
					&error);
	ck_assert(error != NULL);
	ufa_error_print_and_free(error);
	ck_assert(c == false);
}
END_TEST

START_TEST(api_listtags_ok)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags = NULL;

	// Add TAGS
	for (UFA_ARRAY_EACH(x, TAGS)) {
		ufa_repo_inserttag(global_repo, TAGS[x], NULL);
	}

	// Get list of tags
	tags = ufa_jsonrpc_api_listtags(api, TMP_REPO_DIR, &error);
	ck_assert(error == NULL);
	ck_assert_int_eq(ufa_list_size(tags), ARRAY_SIZE(TAGS));

	// Check if all tags have been added
	for (UFA_ARRAY_EACH(x, TAGS)) {
		ck_assert(ufa_list_find_by_data(
		    tags, TAGS[x], (ufa_list_equal_fn_t) ufa_str_equals));
	}

	// Check a non-existent tag
	ck_assert(!ufa_list_find_by_data(tags,
					 "NONEXISTENT",
					 (ufa_list_equal_fn_t)ufa_str_equals));
	ufa_list_free(tags);
}
END_TEST

START_TEST(api_listtags_ok_empty)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags = NULL;

	tags = ufa_jsonrpc_api_listtags(api, TMP_REPO_DIR, &error);
	ck_assert(error == NULL);
	ck_assert_int_eq(ufa_list_size(tags), 0);
	ck_assert(!ufa_list_find_by_data(tags,
					 "NONEXISTENT",
					 (ufa_list_equal_fn_t)ufa_str_equals));

	ufa_list_free(tags);
}
END_TEST

START_TEST(api_listtags_not_dir)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags = NULL;

	tags = ufa_jsonrpc_api_listtags(api, TMP_TEST_FILE_NOTFOUND, &error);

	ck_assert(error != NULL);
	ck_assert(tags == NULL);
	ck_assert(error->code == JSONRPC_INTERNAL_ERROR);

	ufa_error_free(error);
}
END_TEST

START_TEST(api_listtags_corrupt_db)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags = NULL;

	ufa_repo_inserttag(global_repo, "test", NULL);
	corrupt_db();

	tags = ufa_jsonrpc_api_listtags(api, TMP_REPO_DIR, &error);
	ck_assert(error != NULL);
	ck_assert(tags == NULL);
	ck_assert(error->code == JSONRPC_INTERNAL_ERROR);

	ufa_error_free(error);
}
END_TEST

START_TEST(api_gettags_ok)
{
	struct ufa_list *tags = NULL;
	struct ufa_error *error = NULL;

	bool r = ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG1, NULL);
	r = r && ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG2, NULL);
	r = r && ufa_jsonrpc_api_gettags(api, TMP_TEST_FILE1, &tags, &error);
	ufa_error_print(error);
	ck_assert(r == true);

	ck_assert(error == NULL);
	ck_assert(tags != NULL);
	ck_assert(ufa_list_find_by_data(tags,
					TAG1,
					(ufa_list_equal_fn_t) ufa_str_equals));
	ck_assert(ufa_list_find_by_data(tags,
					TAG2,
					(ufa_list_equal_fn_t) ufa_str_equals));
	ck_assert_int_eq(ufa_list_size(tags), 2);

	ufa_list_free(tags);
}
END_TEST

START_TEST(api_inserttag_ok)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags = NULL;

	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG1, NULL);

	int tag_id = ufa_jsonrpc_api_inserttag(api, TMP_REPO_DIR, TAG1, &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert(tag_id > 0);

	tags = ufa_jsonrpc_api_listtags(api, TMP_REPO_DIR, &error);
	ck_assert_int_eq(1, ufa_list_size(tags));
	ck_assert(ufa_list_find_by_data(tags,
					TAG1,
					(ufa_list_equal_fn_t) ufa_str_equals));

	ufa_list_free(tags);
}
END_TEST

START_TEST(api_cleartags_ok)
{
	struct ufa_error *error = NULL;
	struct ufa_list *tags_before = NULL;
	struct ufa_list *tags_after = NULL;

	// Set tags
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG1, NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG2, NULL);

	// Check tags added
	ufa_jsonrpc_api_gettags(api, TMP_TEST_FILE1, &tags_before, &error);
	ck_assert_int_eq(2, ufa_list_size(tags_before));
	ck_assert(ufa_list_find_by_data(tags_before,
					TAG1,
					(ufa_list_equal_fn_t) ufa_str_equals));
	ck_assert(ufa_list_find_by_data(tags_before,
					TAG2,
					(ufa_list_equal_fn_t) ufa_str_equals));
	ufa_list_free(tags_before);

	// Clear tags
	bool result = ufa_jsonrpc_api_cleartags(api, TMP_TEST_FILE1, &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert(result == true);

	// Ensure tag list is empty
	ufa_jsonrpc_api_gettags(api, TMP_TEST_FILE1, &tags_after, &error);
	ck_assert_int_eq(ufa_list_size(tags_after), 0);
	ufa_list_free(tags_after);
}
END_TEST

START_TEST(api_unsettags_ok)
{
	struct ufa_error *error      = NULL;
	struct ufa_list *tags_before = NULL;
	struct ufa_list *tags_after  = NULL;

	// Set tags
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG1, NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, TAG2, NULL);

	// Check tags added
	tags_before = NULL;
	ufa_jsonrpc_api_gettags(api, TMP_TEST_FILE1, &tags_before, &error);
	ck_assert_int_eq(2, ufa_list_size(tags_before));
	ufa_list_free(tags_before);

	// Unset tag
	bool result = ufa_jsonrpc_api_unsettag(api,
					       TMP_TEST_FILE1,
					       TAG1,
					       &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert(result == true);

	// Ensure tag is not set
	ufa_jsonrpc_api_gettags(api, TMP_TEST_FILE1, &tags_after, &error);
	ck_assert_int_eq(1, ufa_list_size(tags_after));
	ck_assert(ufa_list_find_by_data(tags_after,
					TAG2,
					(ufa_list_equal_fn_t) ufa_str_equals));
	ufa_list_free(tags_after);
}
END_TEST

START_TEST(api_unsettags_filenotfound)
{
	struct ufa_error *error = NULL;

	// Unset tag
	bool result = ufa_jsonrpc_api_unsettag(api,
					       TMP_TEST_FILE_NOTFOUND,
					       TAG1,
					       &error);
	ck_assert(error != NULL);
	ck_assert(result == false);
	ufa_error_print_and_free(error);
}
END_TEST

/* ========================================================================== */
/* TEST FUNCTIONS FOR ATTRIBUTE MANAGEMENT                                    */
/* ========================================================================== */

START_TEST(api_setattr_ok)
{
	struct ufa_error *error = NULL;
	bool result = ufa_jsonrpc_api_setattr(api,
					      TMP_TEST_FILE1,
					      "author",
					      "me",
					      &error);
	ck_assert(result == true);
	ck_assert(error == NULL);
}
END_TEST

START_TEST(api_setattr_filenotfound)
{
	struct ufa_error *error = NULL;
	bool result = ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE_NOTFOUND,
					      "author",
					      "me",
					      &error);
	ck_assert(result == false);
	ck_assert(error != NULL);
	ck_assert_int_eq(JSONRPC_INTERNAL_ERROR, error->code);
	ufa_error_free(error);

}
END_TEST

START_TEST(api_getattr_ok)
{
	struct ufa_error *error = NULL;
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE1, "author", "me", NULL);
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE1, "year", "2009", NULL);

	struct ufa_list *attrs = ufa_jsonrpc_api_getattr(api,
							 TMP_TEST_FILE1,
							 &error);
	ufa_error_print(error);
	ck_assert_int_eq(2, ufa_list_size(attrs));

	// Check values
	bool author = false;
	bool year = false;
	for (UFA_LIST_EACH(i, attrs)) {
		struct ufa_repo_attr *attr = i->data;
		if (ufa_str_equals(attr->attribute, "author")) {
			ck_assert_str_eq(attr->value, "me");
			author = true;
		} else if (ufa_str_equals(attr->attribute, "year")) {
			ck_assert_str_eq(attr->value, "2009");
			year = true;
		} else {
			ck_assert(false);
		}
	}
	ck_assert(author);
	ck_assert(year);

	ufa_list_free(attrs);
}
END_TEST

START_TEST(api_getattr_empty_ok)
{
	struct ufa_error *error = NULL;

	struct ufa_list *attr = ufa_jsonrpc_api_getattr(api,
							TMP_TEST_FILE1,
							&error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert_int_eq(0, ufa_list_size(attr));
}
END_TEST

START_TEST(api_unsetattr_ok)
{
	struct ufa_error *error = NULL;

	// Set attributes on file
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE1, "author", "me", NULL);
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE1, "year", "2009", NULL);

	// Check attributes added
	struct ufa_list *attrs =
	    ufa_jsonrpc_api_getattr(api, TMP_TEST_FILE1, &error);
	ck_assert_int_eq(ufa_list_size(attrs), 2);
	ufa_list_free_full(attrs, (ufa_list_free_fn_t) ufa_repo_attr_free);


	// Unset one attribute and check the other
	ufa_jsonrpc_api_unsetattr(api, TMP_TEST_FILE1, "year", &error);
	ufa_error_print(error);
	ck_assert(error == NULL);

	attrs = ufa_jsonrpc_api_getattr(api, TMP_TEST_FILE1, &error);
	ck_assert_int_eq(ufa_list_size(attrs), 1);
	struct ufa_repo_attr *attr = attrs->data;
	ck_assert_str_eq(attr->attribute, "author");
	ck_assert_str_eq(attr->value, "me");
	ufa_list_free_full(attrs, (ufa_list_free_fn_t) ufa_repo_attr_free);

	// Unset the other attribute
	ufa_jsonrpc_api_unsetattr(api, TMP_TEST_FILE1, "author", NULL);
	ck_assert(ufa_jsonrpc_api_getattr(api, TMP_TEST_FILE1, &error) == NULL);
}
END_TEST

/* ========================================================================== */
/* TEST FUNCTIONS FOR SEARCH CAPABILITIES                                     */
/* ========================================================================== */

START_TEST(api_search_attr_filenotfound)
{
	struct ufa_error *error = NULL;
	bool f = ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE_NOTFOUND,
					 "year", "2010", &error);
	ufa_error_print_and_free(error);
	ck_assert(f == false);
}
END_TEST


START_TEST(api_search_attr_ok)
{
	struct ufa_error *error      = NULL;
	struct ufa_list *list_attrs  = NULL;
	struct ufa_list *list_tags   = NULL;
	struct ufa_list *list_attrs2 = NULL;
	struct ufa_list *repo_dirs   = ufa_list_append(NULL,
						     ufa_str_dup(TMP_REPO_DIR));

	// Set attributes on file 1
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE1, "author", "me", NULL);
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE1, "year", "2009", NULL);

	// Set attributes on file 2
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE2, "year", "2010", NULL);


	struct ufa_repo_filterattr *attr_author =
	    ufa_repo_filterattr_new("author", "me", UFA_REPO_EQUAL);
	list_attrs = ufa_list_append(list_attrs, attr_author);

	struct ufa_repo_filterattr *attr_year =
	    ufa_repo_filterattr_new("year", "2009", UFA_REPO_EQUAL);
	list_attrs = ufa_list_append(list_attrs, attr_year);

	struct ufa_repo_filterattr *attr_year2 =
	    ufa_repo_filterattr_new("year", "20*", UFA_REPO_WILDCARD);
	list_attrs2 = ufa_list_append(list_attrs2, attr_year2);

	struct ufa_list *value = ufa_jsonrpc_api_search(api,
							repo_dirs,
							list_attrs,
							list_tags,
							false,
							&error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert(value != NULL);
	ck_assert_int_eq(ufa_list_size(value), 1);
	ck_assert_str_eq(TMP_TEST_FILE1, value->data);

	struct ufa_list *value2 = ufa_jsonrpc_api_search(api,
							 repo_dirs,
							 list_attrs2,
							 list_tags,
							 false,
							 &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert(value2 != NULL);
	ck_assert_int_eq(ufa_list_size(value2), 2);

	ck_assert(ufa_list_find_by_data(value2,
					TMP_TEST_FILE1,
					(ufa_list_equal_fn_t) ufa_str_equals));
	ck_assert(ufa_list_find_by_data(value2,
					TMP_TEST_FILE2,
					(ufa_list_equal_fn_t) ufa_str_equals));

	ufa_list_free(value);
	ufa_list_free(value2);
	ufa_list_free_full(repo_dirs, ufa_free);
}
END_TEST


START_TEST(api_search_tags_ok)
{
	struct ufa_error *error     = NULL;
	struct ufa_list *list_attrs = NULL;
	struct ufa_list *list_tags  = NULL;
	struct ufa_list *repo_dirs  = ufa_list_append(NULL,
						     ufa_str_dup(TMP_REPO_DIR));

	// Set tags on file 1
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "math", NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "calculus", NULL);

	// Set tags on file 2
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE2, "math", NULL);

	list_tags = ufa_list_append(list_tags, "math");

	struct ufa_list *result = ufa_jsonrpc_api_search(api,
							 repo_dirs,
							 list_attrs,
							 list_tags,
							 false,
							 &error);

	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert(result != NULL);
	ck_assert_int_eq(ufa_list_size(result), 2);
	ck_assert(ufa_list_find_by_data(
	    result, TMP_TEST_FILE1, (ufa_list_equal_fn_t) ufa_str_equals));
	ck_assert(ufa_list_find_by_data(
	    result, TMP_TEST_FILE2, (ufa_list_equal_fn_t) ufa_str_equals));

	ufa_list_free(list_attrs);
	ufa_list_free(list_tags);
	ufa_list_free(result);
}
END_TEST


START_TEST(api_search_tags_multiple_ok)
{
	struct ufa_error *error     = NULL;
	struct ufa_list *list_attrs = NULL;
	struct ufa_list *list_tags  = NULL;
	struct ufa_list *repo_dirs  = ufa_list_append(NULL,
						     ufa_str_dup(TMP_REPO_DIR));

	// Set tags on file 1
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "math", NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "calculus", NULL);

	// Set tags on file 2
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE2, "other", NULL);

	list_tags = ufa_list_append(list_tags, "math");
	list_tags = ufa_list_append(list_tags, "calculus");

	struct ufa_list *result = ufa_jsonrpc_api_search(api,
							 repo_dirs,
							 list_attrs,
							 list_tags,
							 false,
							 &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert_int_eq(ufa_list_size(result), 1);
	ck_assert(ufa_list_find_by_data(result, TMP_TEST_FILE1,
					(ufa_list_equal_fn_t) ufa_str_equals));

	ufa_list_free(list_tags);
	ufa_list_free(result);
}
END_TEST

START_TEST(api_search_tags_multiple_notfound_ok)
{
	struct ufa_error *error     = NULL;
	struct ufa_list *list_attrs = NULL;
	struct ufa_list *list_tags  = NULL;
	struct ufa_list *repo_dirs  = ufa_list_append(NULL,
						     ufa_str_dup(TMP_REPO_DIR));

	// Set tags on file 1
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "math", NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "calculus", NULL);

	// Set tags on file 2
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE2, "other", NULL);


	list_tags = ufa_list_append(list_tags, "math");
	list_tags = ufa_list_append(list_tags, "other");

	struct ufa_list *result = ufa_jsonrpc_api_search(api,
							 repo_dirs,
							 list_attrs,
							 list_tags,
							 false,
							 &error);
	ufa_error_print(error);
	ck_assert(error == NULL);
	ck_assert_int_eq(ufa_list_size(result), 0);

	ufa_list_free(list_tags);
	ufa_list_free(result);
}
END_TEST


START_TEST(api_search_tags_and_attrs_ok)
{
	struct ufa_error *error     = NULL;
	struct ufa_list *list_attrs = NULL;
	struct ufa_list *list_tags  = NULL;
	struct ufa_list *repo_dirs  = ufa_list_append2(NULL,
						      ufa_str_dup(TMP_REPO_DIR),
						      ufa_free);

	// Set tags on file 1
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "math", NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE1, "calculus", NULL);

	// Set tags and attrs on file 2
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE2, "math", NULL);
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE2, "author", "me", NULL);

	// Set tags and attrs on file 3
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE3, "math", NULL);
	ufa_jsonrpc_api_settag(api, TMP_TEST_FILE3, "calculus", NULL);
	ufa_jsonrpc_api_setattr(api, TMP_TEST_FILE3, "author", "me", NULL);


	// Setting up search for tags 'math', 'calculus', and attribute 'author'
	list_tags = ufa_list_append(list_tags, "math");
	list_tags = ufa_list_append(list_tags, "calculus");
	struct ufa_repo_filterattr *attr_year = ufa_repo_filterattr_new("author", "me", UFA_REPO_EQUAL);
	list_attrs = ufa_list_append2(list_attrs,
				      attr_year,
				      (ufa_list_free_fn_t) ufa_repo_filterattr_free);


	struct ufa_list *result = ufa_jsonrpc_api_search(api,
							 repo_dirs,
							 list_attrs,
							 list_tags,
							 false,
							 &error);
	ufa_error_print_and_free(error);
	ck_assert(error == NULL);
	ck_assert(result != NULL);
	ck_assert_int_eq(ufa_list_size(result), 1);
	ck_assert(ufa_list_find_by_data(result,
					TMP_TEST_FILE3,
					(ufa_list_equal_fn_t) ufa_str_equals));

	ufa_list_free(list_attrs);
	ufa_list_free(repo_dirs);
	ufa_list_free(list_tags);
	ufa_list_free(result);
}
END_TEST

/* ========================================================================== */
/* SUITE DEFINITIONS AND MAIN FUNCTION                                        */
/* ========================================================================== */

Suite *repo_suite(void)
{
	Suite *s;
	TCase *tc_tag;
	TCase *tc_attr;
	TCase *tc_search;

	s = suite_create("API");

	/* TAG test case */
	tc_tag = tcase_create("tag");
	tcase_add_checked_fixture(tc_tag, setup_repo, teardown_repo);
	tcase_add_test(tc_tag, api_settag);
	tcase_add_test(tc_tag, api_settag_nonexistent_file);
	tcase_add_test(tc_tag, api_listtags_ok);
	tcase_add_test(tc_tag, api_listtags_ok_empty);
	tcase_add_test(tc_tag, api_listtags_not_dir);
	tcase_add_test(tc_tag, api_listtags_corrupt_db);
	tcase_add_test(tc_tag, api_gettags_ok);
	tcase_add_test(tc_tag, api_inserttag_ok);
	tcase_add_test(tc_tag, api_cleartags_ok);
	tcase_add_test(tc_tag, api_unsettags_ok);
	tcase_add_test(tc_tag, api_unsettags_filenotfound);

	/* ATTRIBUTE test case */
	tc_attr = tcase_create("attribute");
	tcase_add_checked_fixture(tc_attr, setup_repo, teardown_repo);
	tcase_add_test(tc_attr, api_setattr_ok);
	tcase_add_test(tc_attr, api_setattr_filenotfound);
	tcase_add_test(tc_attr, api_getattr_ok);
	tcase_add_test(tc_attr, api_getattr_empty_ok);
	tcase_add_test(tc_attr, api_unsetattr_ok);

	/* SEARCH test case */
	tc_search = tcase_create("search");
	tcase_add_checked_fixture(tc_search, setup_repo, teardown_repo);

	tcase_add_test(tc_search, api_search_attr_filenotfound);
	tcase_add_test(tc_search, api_search_attr_ok);
	tcase_add_test(tc_search, api_search_tags_ok);
	tcase_add_test(tc_search, api_search_tags_multiple_ok);
	tcase_add_test(tc_search, api_search_tags_multiple_notfound_ok);
	tcase_add_test(tc_search, api_search_tags_and_attrs_ok);

	/* Add test cases to suite */
	suite_add_tcase(s, tc_tag);
	suite_add_tcase(s, tc_attr);
	suite_add_tcase(s, tc_search);

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