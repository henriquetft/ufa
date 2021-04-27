/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of repo module (repo.h) using sqlite3.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "logging.h"
#include "list.h"
#include "misc.h"
#include "repo.h"

#define STR_CREATE_TABLE \
"CREATE TABLE IF NOT EXISTS \"attribute\" ( \n"\
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"id_file\"	INTEGER, \n"\
	"\"name\"	TEXT, \n"\
	"\"value\"	TEXT, \n"\
	"FOREIGN KEY(\"id_file\") REFERENCES \"file\"(\"id\") \n"\
");\n"\
"CREATE TABLE IF NOT EXISTS \"file_tag\" (\n" \
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"id_file\"	INTEGER, \n"\
	"\"id_tag\"	INTEGER, \n"\
	"FOREIGN KEY(\"id_file\") REFERENCES \"file\"(\"id\"), \n"\
	"FOREIGN KEY(\"id_tag\") REFERENCES \"tag\"(\"id\") \n"\
"); \n"\
"CREATE TABLE IF NOT EXISTS \"file\" ( \n"\
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"name\"	TEXT \n"\
"); \n"\
"CREATE TABLE IF NOT EXISTS \"tag\" ( \n"\
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"name\"	TEXT \n"\
"); \n"


#define REPOSITORY_FILENAME "repo.sqlite"

typedef struct ufa_repo_conn_s {
    sqlite3 *db; /* sqlite3 object */
    char *name; /* name of the file */
} ufa_repo_conn_t;


static char *repository_path = NULL;
static ufa_repo_conn_t *conn;


static char *
get_filename(const char *filepath)
{
    ufa_list_t *split = ufa_util_str_split(filepath, "/");
    ufa_list_t *last = ufa_list_get_last(split);
    char *last_part = ufa_strdup((char *)last->data);
    ufa_list_free_full(split, free);
    return last_part;
}


static ufa_repo_conn_t *
ufa_repo_open(const char *file)
{
    char *errmsg = NULL;
    ufa_repo_conn_t *conn = malloc(sizeof *conn);
    int rc = sqlite3_open(file, &conn->db); /* create file if it do not exist */

    if (rc != SQLITE_OK) {
        goto error_opening;
    }
    struct stat st;
    if (stat(file, &st) != 0) {
        goto error_stat;
    }

    /* if new file, create tables */
    if (st.st_size == 0) {
        ufa_debug("creating tables ...");
        rc = sqlite3_exec(conn->db, STR_CREATE_TABLE, NULL, 0, &errmsg);
        if (rc != SQLITE_OK) {
            goto error_create_table;
        }
    }

    conn->name = ufa_strdup(file);
    return conn;

error_opening:
    free(conn);
    ufa_error("Error opening SQLite db %s. Returned: %d", file, rc);
    return NULL;
error_stat:
    free(conn);
    perror("stat");
    return NULL;
error_create_table:
    ufa_error("SQLite3 error creating table: (%d) - %s", rc, errmsg);
    sqlite3_free(errmsg);
    sqlite3_close(conn->db);
    free(conn);
    return NULL;
}


static ufa_list_t *
ufa_get_all_tags()
{
    ufa_list_t *all_tags = NULL;

    int rows = 0, columns = 0;
    char **result_sql = NULL;
    char *err = NULL;

    char *sql = sqlite3_mprintf("SELECT name FROM tag");

    int sql_ret = sqlite3_get_table(conn->db, sql, &result_sql, &rows, &columns, &err);

    if (sql_ret) {
        goto sqlite_error;
    }

    int i;
    for (i = columns; i < (rows + 1) * columns;) {
        // first line is column headers, so we add +1
        char *tag = ufa_strdup(result_sql[i++]);
        all_tags = ufa_list_append(all_tags, tag);
    }

    sqlite3_free(sql);
    sqlite3_free_table(result_sql);

    return ufa_list_get_first(all_tags);

    /* error handling */
sqlite_error:
    if (all_tags) {
        ufa_list_t *head = ufa_list_get_first(all_tags);
        ufa_list_free_full(head, free);
    }
    ufa_error("Sqlite3 error: %d (%s)", sql_ret, err);
    sqlite3_free(err);
    sqlite3_free(sql);
    return NULL;
}


static ufa_list_t *
get_tags_for_files_excluding(ufa_list_t *file_ids, ufa_list_t *tags)
{
    int number_of_tags = ufa_list_size(tags);
    int number_of_files = ufa_list_size(file_ids);

    if (number_of_files == 0) {
        return NULL;
    }

    ufa_list_t *result = NULL;

    char *sql = "SELECT DISTINCT t.name FROM tag t, file_tag ft, file f WHERE ft.id_tag = t.id AND "
                "f.id = ft.id_file";
    char *sql_file_args = ufa_util_str_multiply("?,", number_of_files);
    sql_file_args[strlen(sql_file_args) - 1] = '\0';

    char *sql_tags_args;
    if (number_of_tags) {
        sql_tags_args = ufa_util_str_multiply("?,", number_of_tags);
        sql_tags_args[strlen(sql_tags_args) - 1] = '\0';
    } else {
        sql_tags_args = ufa_strdup("");
    }

    char *full_sql = ufa_util_join_str(
        "", 6, sql, " AND f.id IN (", sql_file_args, ") AND t.name NOT IN (", sql_tags_args, ")");
    ufa_debug("Query: %s", full_sql);
    free(sql_file_args);
    free(sql_tags_args);

    sqlite3_stmt *stmt;
    int cmdStat = sqlite3_prepare_v2(conn->db, full_sql, -1, &stmt, NULL);

    if (cmdStat != SQLITE_OK) {
        goto end;
    }

    /* setting query args */
    int x = 1;
    for (ufa_list_t *list = file_ids; (list != NULL); list = list->next) {
        sqlite3_bind_int(stmt, x++, *((int *)list->data));
    }

    for (ufa_list_t *list = tags; (list != NULL); list = list->next) {
        sqlite3_bind_text(stmt, x++, (char *)list->data, -1, NULL);
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        const char *tag = (const char *)sqlite3_column_text(stmt, 0);
        result = ufa_list_append(result, ufa_strdup(tag));
    }
end:
    sqlite3_finalize(stmt);
    free(full_sql);
    return result;
}


static ufa_list_t *
get_files_with_tags(ufa_list_t *tags)
{
    ufa_list_t *result = NULL;
    ufa_list_t *file_ids = NULL;

    char *sql = "SELECT DISTINCT f.id,f.name FROM tag t, file_tag ft, file f WHERE ft.id_tag = "
                "t.id AND f.id = "
                "ft.id_file AND t.name";

    unsigned int size = ufa_list_size(tags);

    char *sql_args = ufa_util_str_multiply("?,", size);
    sql_args[strlen(sql_args) - 1] = '\0';

    char *full_sql = ufa_util_join_str("", 4, sql, " IN (", sql_args, ")");

    ufa_debug("Executing query: %s", full_sql);

    sqlite3_stmt *stmt;
    int cmdStat = sqlite3_prepare_v2(conn->db, full_sql, -1, &stmt, NULL);

    if (cmdStat != SQLITE_OK) {
        goto end;
    }

    ufa_list_t *list = tags;
    int x = 1;
    for (; (list != NULL); list = list->next) {
        sqlite3_bind_text(stmt, x++, (char *)list->data, -1, NULL);
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        int stmt_id = sqlite3_column_int(stmt, 0);
        int *id = malloc(sizeof(int));
        *id = stmt_id;
        const char *filename = (const char *)sqlite3_column_text(stmt, 1);

        result = ufa_list_append(result, ufa_strdup(filename));
        file_ids = ufa_list_append(file_ids, id);
    }

    // for the files selected get all tags that are not in list 'tags'
    // because we need to show files with and tags other than the ones in path
    ufa_list_t *other_tags = get_tags_for_files_excluding(file_ids, tags);

    if (result && other_tags) {
        /* concatenate lists */
        ufa_list_get_last(result)->next = other_tags;
    }
end:
    sqlite3_finalize(stmt);
    ufa_list_free_full(file_ids, free);
    free(sql_args);
    free(full_sql);
    return result;
}


int
ufa_repo_init(char *repository)
{
    if (!ufa_util_isdir(repository)) {
        return -1;
    }
    repository_path = ufa_strdup(repository);
    char *filepath = ufa_util_join_path(2, repository_path, REPOSITORY_FILENAME);
    ufa_debug("Initializing repo %s", filepath);
    conn = ufa_repo_open(filepath);
    free(filepath);
    return 0;
}


int
ufa_repo_is_a_tag(const char *path)
{
    /* e.g.: tag1/tag2/tag3 or tag1/tag2/file. */
    return ufa_repo_get_file_path(path) == NULL;
}


char *
ufa_repo_get_file_path(const char *path)
{
    ufa_list_t *split = ufa_util_str_split(path, "/");
    ufa_list_t *last = ufa_list_get_last(split);
    char *last_part = (char *)last->data;

    char *filepath;
    filepath = (char *)calloc(
        strlen(repository_path) + strlen("/") + strlen(last->data) + 1, sizeof *filepath);
    strcpy(filepath, repository_path);
    strcat(filepath, "/");
    strcat(filepath, last_part);

    if (ufa_util_isfile(filepath)) {
        ufa_list_free_full(split, free);
        return filepath;
    } else {
        free(filepath);
        ufa_list_free_full(split, free);
        return NULL;
    }
}


ufa_list_t *
ufa_repo_list_files_for_dir(const char *path)
{
    ufa_list_t *list = NULL;
    if (ufa_util_strequals(path, "/")) {
        /* FIXME cache. it is better to clone all_tags */
        return ufa_get_all_tags();
    } else {
        ufa_list_t *list_of_tags = ufa_util_str_split(path, "/");
        // get all files with tags
        list = get_files_with_tags(list_of_tags);
        ufa_list_free_full(list_of_tags, free);
    }
    return list;
}


ufa_list_t *
ufa_repo_get_tags_for_file(const char *filepath)
{
    ufa_list_t *result = NULL;

    char *filename = get_filename(filepath);
    ufa_debug("Listing tags for filename: %s (%s)", filename, filepath);
    char *sql = "SELECT DISTINCT t.name FROM file f , file_tag ft, tag t where f.id = ft.id_file "
                "and ft.id_tag = t.id and f.name=? ORDER BY t.name";


    sqlite3_stmt *stmt = NULL;
    int prepare_ret = sqlite3_prepare_v2(conn->db, sql, -1, &stmt, NULL);

    if (prepare_ret != SQLITE_OK) {
        goto end;
    }

    sqlite3_bind_text(stmt, 1, filename, -1, NULL);

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        const char *tag = (const char *)sqlite3_column_text(stmt, 0);
        result = ufa_list_append(result, ufa_strdup(tag));
    }
end:
    sqlite3_finalize(stmt);
    free(filename);
    return result;
}