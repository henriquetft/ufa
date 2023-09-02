/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Test cases for jsonrpc_parser.c                                            */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/list.h"
#include "util/misc.h"
#include "json/jsonrpc_parser.h"
#include <check.h>
#include <stdlib.h>

/* ========================================================================== */
/* TEST FUNCTIONS                                                             */
/* ========================================================================== */

START_TEST(full_ok)
{
	char *vet[2];
	vet[0] = "{ "
		 "  \"params\" : { \"filepath\" : \"fileA\", \"attr\" : "
		 "\"unix\", \"size\" : 543, \"enabled\" : true, \"items\" : [ "
		 "1, 2.9, \"test str\", false, null ]   },\n"
		 "\"jsonrpc\" : \"2.0\",\n"
		 "  \"method\" : \"settag\",\n"
		 "  \"id\" : \"1\"\n"
		 "}";

	vet[1] = "{ "
		 "\"jsonrpc\" : \"2.0\",\n"
		 "\"tobypass\" : \"hello ;)\",\n"
		 "  \"id\" : \"1\",\n"
		 "  \"method\" : \"settag\",\n"
		 "  \"params\" : { \"filepath\" : \"fileA\", \"attr\" : "
		 "\"unix\", \"size\" : 543, \"enabled\" : true, \"items\" : [ "
		 "1, 2.9, \"test str\", false, null ]   }\n"
		 "}";

	for (UFA_ARRAY_EACH(x, vet)) {
		struct ufa_jsonrpc *rpc = NULL;
		ck_assert(ufa_jsonrpc_parse(vet[1], &rpc) == UFA_JSON_OK);
		ck_assert_str_eq(rpc->method, "settag");
		ck_assert_str_eq(rpc->id, "1");
		ck_assert(rpc->params != NULL);
		ck_assert_str_eq(ufa_hashtable_get(rpc->params, "filepath"),
				 "fileA");
		ck_assert_str_eq(ufa_hashtable_get(rpc->params, "attr"),
				 "unix");
		ck_assert_int_eq(
		    *((long *)ufa_hashtable_get(rpc->params, "size")), 543);
		ck_assert_int_eq(
		    *((bool *)ufa_hashtable_get(rpc->params, "enabled")), true);

		struct ufa_list *items =
		    ufa_hashtable_get(rpc->params, "items");
		ck_assert(items != NULL);
		ck_assert_int_eq(*((long *) items->data), 1L);

		items = items->next;
		ck_assert(*((double *) items->data) == 2.9);

		items = items->next;
		ck_assert_str_eq((char *) items->data, "test str");

		items = items->next;
		ck_assert_int_eq(*((bool *) items->data), false);

		items = items->next;
		ck_assert(((void *) items->data) == NULL);

		ufa_jsonrpc_free(rpc);
	}

}
END_TEST

START_TEST(no_params_ok)
{
	char *vet[2];

	vet[0] = "{"
		 "   \"jsonrpc\": \"2.0\","
		 "   \"method\": \"listtags\","
		 "   \"id\" : \"hoho-123\""
		 "}";

	vet[1] = "{"
		 "   \"jsonrpc\": \"2.0\","
		 "   \"method\": \"listtags\","
		 "   \"id\" : \"hoho-123\","
		 "   \"params\" : { } "
		 "}";

	for (UFA_ARRAY_EACH(x, vet)) {
		struct ufa_jsonrpc *rpc = NULL;
		ck_assert(ufa_jsonrpc_parse(vet[x], &rpc) == UFA_JSON_OK);

		ck_assert_str_eq(rpc->method, "listtags");
		ck_assert_str_eq(rpc->id, "hoho-123");
		ck_assert(rpc->params != NULL);
		ck_assert_int_eq(ufa_hashtable_size(rpc->params), 0);

		ufa_jsonrpc_free(rpc);
	}
}
END_TEST


START_TEST(invalid_json1_error)
{
	char *json_str = "{ ]";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_INVAL);
}
END_TEST


START_TEST(partial_json_error)
{

	char *json_str = "{"
			 "   \"jsonrpc\": \"2.0\","
			 "   \"id\" : \"hoho-123\","
			 "   \"params\" : { \"file";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert_int_eq(UFA_JSON_PART, ufa_jsonrpc_parse(json_str, &rpc));
}
END_TEST



START_TEST(no_id_ok)
{
	char *json_str;

	json_str = "{"
		 "   \"jsonrpc\": \"2.0\","
		 "   \"method\": \"listtags\""
		 "}";

	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert_str_eq(rpc->method, "listtags");
	ck_assert(rpc->id == NULL);
	ck_assert(rpc->params != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));

	ufa_jsonrpc_free(rpc);
}
END_TEST


// TODO how to know if there are characters remaining ?
START_TEST(two_obj_one_partial)
{
	char *json_str;

	json_str = "{"
		   "   \"jsonrpc\": \"2.0\","
		   "   \"method\": \"listtags\""
		   "}"
		   "{ \"abc\": \"aaa\" }";

	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert_str_eq(rpc->method, "listtags");
	ck_assert(rpc->id == NULL);
	ck_assert(rpc->params != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));

	ufa_jsonrpc_free(rpc);
}
END_TEST

START_TEST(resopnse_bool)
{
	const char *json_str = "{ \"jsonrpc\" : \"2.0\", \"id\" : \"xpto-123\","
			       "\"result\" : { \"value\" : true } }";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert(rpc->id != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(1, ufa_hashtable_size(rpc->result));

	ck_assert_int_eq(
	    *((bool *) ufa_hashtable_get(rpc->result, "value")), true);

	ufa_jsonrpc_free(rpc);
}
END_TEST


START_TEST(resopnse_list)
{
	const char *json_str =
	    "{ \"jsonrpc\" : \"2.0\", \"id\" : \"xpto-123\", "
	    "\"result\" : { \"value\" : [ \"tag1\", \"tag2\" ] } }";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert(rpc->id != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(1, ufa_hashtable_size(rpc->result));

	struct ufa_list *items = ufa_hashtable_get(rpc->result, "value");
	ck_assert(items != NULL);

	ck_assert_str_eq((char *) items->data, "tag1");

	items = items->next;
	ck_assert_str_eq((char *) items->data, "tag2");

	ufa_jsonrpc_free(rpc);
}
END_TEST

START_TEST(resopnse_obj)
{
	const char *json_str =
	    "{ \"jsonrpc\" : \"2.0\", \"id\" : \"xpto-123\", "
	    "\"result\" : { \"value\" : { \"attr1\" : \"value1\", \"attr2\" : "
	    "\"value2\" } } }";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert(rpc->id != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(1, ufa_hashtable_size(rpc->result));

	ufa_hashtable_t *value = ufa_hashtable_get(rpc->result, "value");
	ck_assert(value != NULL);
	struct ufa_list *list_keys = NULL;
	struct ufa_list *keys = list_keys = ufa_hashtable_keys(value);
	ck_assert_int_eq(2, ufa_hashtable_size(value));
	ck_assert_int_eq(2, ufa_list_size(keys));

	ck_assert_str_eq((char *) keys->data, "attr1");

	keys = keys->next;
	ck_assert_str_eq((char *) keys->data, "attr2");

	ck_assert_str_eq((char *) ufa_hashtable_get(value, "attr1"), "value1");
	ck_assert_str_eq((char *) ufa_hashtable_get(value, "attr2"), "value2");

	ufa_list_free(list_keys);
	ufa_jsonrpc_free(rpc);
}
END_TEST

START_TEST(resopnse_list_obj)
{
	const char *json_str =
	    "{ \"jsonrpc\" : \"2.0\", \"id\" : \"xpto-123\", "
	    "\"result\" : { \"value\" : [ { \"attr1\" : \"value1\", \"attr2\" "
	    ": \"value2\" } ] } }";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert(rpc->id != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(1, ufa_hashtable_size(rpc->result));

	struct ufa_list *value = ufa_hashtable_get(rpc->result, "value");
	ck_assert(value != NULL);
	ck_assert_int_eq(ufa_list_size(value), 1);
	ufa_hashtable_t *values = value->data;
	struct ufa_list *keys = ufa_hashtable_keys(values);
	ck_assert_int_eq(ufa_hashtable_size(values), 2);

	ck_assert_str_eq((char *) keys->data, "attr1");
	ck_assert_str_eq((char *) keys->next->data, "attr2");

	ck_assert_str_eq((char *) ufa_hashtable_get(values, "attr1"), "value1");
	ck_assert_str_eq((char *) ufa_hashtable_get(values, "attr2"), "value2");


	ufa_list_free(keys);
	ufa_jsonrpc_free(rpc);
}
END_TEST

START_TEST(resopnse_list_obj2)
{
	const char *json_str =
	    "{ "
	    "    \"jsonrpc\" : \"2.0\", "
	    "    \"id\" : \"xpto-123\", "
	    "    \"result\" : { "
	    "          \"value\" : [ { \"attr1\" : \"value1\","
	    "                          \"attr2\" : \"value2\" } ],"
	    "          \"list\": [ \"hello\", \"aa\" ],"
	    "          \"list2\" : [ ], "
	    "          \"inside\" : { \"test1\": \"hello\", \"test2\": \"hi\" }"
	    "     } "
	    "}";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert(rpc->id != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(4, ufa_hashtable_size(rpc->result));

	struct ufa_list *value = ufa_hashtable_get(rpc->result, "value");
	ck_assert(value != NULL);
	ck_assert_int_eq(1, ufa_list_size(value));
	ufa_hashtable_t *values = value->data;
	struct ufa_list *keys = ufa_hashtable_keys(values);
	ck_assert_int_eq(2, ufa_hashtable_size(values));

	ck_assert_str_eq((char *) keys->data, "attr1");
	ck_assert_str_eq((char *) keys->next->data, "attr2");

	ck_assert_str_eq((char *) ufa_hashtable_get(values, "attr1"), "value1");
	ck_assert_str_eq((char *) ufa_hashtable_get(values, "attr2"), "value2");

	struct ufa_list *list = ufa_hashtable_get(rpc->result, "list");
	ck_assert(list != NULL);
	ck_assert_int_eq(2, ufa_list_size(list));
	ck_assert_str_eq((char *) list->data, "hello");
	ck_assert_str_eq((char *) list->next->data, "aa");

	// Empty list
	struct ufa_list *list2 = ufa_hashtable_get(rpc->result, "list2");
	ck_assert(list2 == NULL);

	ufa_hashtable_t  *inside_obj = ufa_hashtable_get(rpc->result, "inside");
	ck_assert_int_eq(2, ufa_hashtable_size(inside_obj));
	ck_assert_str_eq("hello", ufa_hashtable_get(inside_obj, "test1"));
	ck_assert_str_eq("hi", ufa_hashtable_get(inside_obj, "test2"));


	ufa_list_free(keys);
	ufa_jsonrpc_free(rpc);
}
END_TEST

START_TEST(resopnse_list_sublist)
{
	const char *json_str =
	    "{ \"jsonrpc\" : \"2.0\", \"id\" : \"xpto-123\", "
	    "\"result\" : { \"value\" : [ { \"attr1\" : \"value1\", \"attr2\" "
	    ": \"value2\" } ], \"list\": [ \"hello\", \"aa\" ], \"list2\" : [ "
	    "\"1\", [100, \"oi\", true], 9, \"5\" ] } }";

	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert(rpc->id != NULL);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(3, ufa_hashtable_size(rpc->result));


	// Test list2
	struct ufa_list *list2 = ufa_hashtable_get(rpc->result, "list2");
	ck_assert_int_eq(4, ufa_list_size(list2));
	ck_assert_str_eq("1",    ufa_list_get(list2, 0)->data);
	ck_assert       (NULL != ufa_list_get(list2, 1)->data);
	ck_assert_int_eq(9,      *((long *) ufa_list_get(list2, 2)->data));
	ck_assert_str_eq("5",    ufa_list_get(list2, 3)->data);

	// Test sublist
	struct ufa_list *sub = (struct ufa_list *) ufa_list_get(list2, 1)->data;
	ck_assert_int_eq(3, ufa_list_size(sub));
	ck_assert_int_eq(100,      *((long *) ufa_list_get(sub, 0)->data));
	ck_assert_str_eq("oi",     ufa_list_get(sub, 1)->data);
	ck_assert_int_eq(true,     *((bool *) ufa_list_get(sub, 2)->data));

	ufa_jsonrpc_free(rpc);
}
END_TEST


START_TEST(resopnse_error)
{
	const char *json_str =
	    "{ \"jsonrpc\" : \"2.0\", \"id\" : \"xpto-123\", "
	    "\"error\" : { \"code\" : -1234, \"message\": \"test msg\" } }";
	struct ufa_jsonrpc *rpc = NULL;
	ck_assert(ufa_jsonrpc_parse(json_str, &rpc) == UFA_JSON_OK);
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->params));
	ck_assert_int_eq(0, ufa_hashtable_size(rpc->result));
	ck_assert_int_eq(2, ufa_hashtable_size(rpc->error));

	ck_assert_int_eq(
	    *((int *) ufa_hashtable_get(rpc->error, "code")), -1234);
	ck_assert_str_eq(
	    (char *) ufa_hashtable_get(rpc->error, "message"), "test msg");

	ufa_jsonrpc_free(rpc);
}
END_TEST

Suite *parser_suite(void)
{
	Suite *s;
	TCase *tc_core;
	TCase *tc_response;

	s = suite_create("parser");

	/* Core test case */
	tc_core = tcase_create("core");
	tcase_add_test(tc_core, full_ok);
	tcase_add_test(tc_core, no_params_ok);
	tcase_add_test(tc_core, no_id_ok);
	tcase_add_test(tc_core, invalid_json1_error);
	tcase_add_test(tc_core, partial_json_error);
	tcase_add_test(tc_core, two_obj_one_partial);

	tc_response = tcase_create("response");
	tcase_add_test(tc_response, resopnse_bool);
	tcase_add_test(tc_response, resopnse_list);
	tcase_add_test(tc_response, resopnse_error);
	tcase_add_test(tc_response, resopnse_obj);
	tcase_add_test(tc_response, resopnse_list_obj);
	tcase_add_test(tc_response, resopnse_list_obj2);
	tcase_add_test(tc_response, resopnse_list_sublist);

	/* Add test cases to suite */
	suite_add_tcase(s, tc_core);
	suite_add_tcase(s, tc_response);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = parser_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}