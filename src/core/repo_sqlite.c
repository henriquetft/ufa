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
#include "util/logging.h"
#include "util/list.h"
#include "util/misc.h"
#include "core/repo.h"
#include "util/error.h"

/* ============================================================================================== */
/* VARIABLES AND DEFINITIONS                                                                      */
/* ============================================================================================== */

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
	"\"name\"	TEXT UNIQUE \n"\
"); \n"\
"CREATE TABLE IF NOT EXISTS \"tag\" ( \n"\
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"name\"	TEXT UNIQUE \n"\
"); \n"\
"CREATE UNIQUE INDEX IF NOT EXISTS \"un\" ON \"file_tag\" (\n"\
	"\"id_file\","\
	"\"id_tag\""\
");"

#define REPOSITORY_FILENAME "repo.sqlite"
#define db_prepare(stmt, sql, error) _db_prepare(stmt, sql, error, __func__)
#define db_execute(stmt, error) _db_execute(stmt, error, __func__)

typedef struct ufa_repo_conn_s {
    sqlite3 *db; /* sqlite3 object */
    char *name;  /* name of the file */
} ufa_repo_conn_t;


static char *repository_path = NULL;
static ufa_repo_conn_t *conn;


/* ============================================================================================== */
/* AUXILIARY FUNCTIONS                                                                            */
/* ============================================================================================== */


bool
_db_prepare(sqlite3_stmt **stmt, char *sql, ufa_error_t **error, const char* func_name)
{
    int prepare_ret = sqlite3_prepare_v2(conn->db, sql, -1, stmt, NULL);
    if (prepare_ret != SQLITE_OK) {
        ufa_error_set(error, UFA_ERROR_DATABASE, "error on function %s:   %s",
            func_name, sqlite3_errmsg(conn->db));
        return false;
    }
    return true;
}


static bool
_db_execute(sqlite3_stmt *stmt, ufa_error_t **error, const char* func_name)
{
    bool status = true;
    int r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
        ufa_error_set(error, UFA_ERROR_DATABASE, "error on function %s:   %s", __func__,
            sqlite3_errmsg(conn->db));
        status = false;
    }
    return status;
}


/**
 * Returns a newly-allocated string with characters '?' separated by ','.
 * The number of characters '?' is equal to the number of elements in list.
 */
static char *
_sql_arg_list(ufa_list_t *list)
{
    int size = ufa_list_size(list);
    if (size == 0) {
        return ufa_strdup("");
    }
    char *sql_args = ufa_util_str_multiply("?,", size);
    sql_args[strlen(sql_args) - 1] = '\0';
    return sql_args;
}


/**
 * Attempts to establish a connection to sqlite repository.
 * Returns the ufa_repo_conn_t or NULL on error. 
 */
static ufa_repo_conn_t *
_open_sqlite_conn(const char *file, ufa_error_t **error)
{
    char *errmsg          = NULL;
    ufa_repo_conn_t *conn = malloc(sizeof *conn);
    int rc                = sqlite3_open(file, &conn->db); /* create file if it do not exist */
    sqlite3_extended_result_codes(conn->db, 1);

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
    ufa_error_set(error, UFA_ERROR_DATABASE, "Error opening SQLite db %s. Returned: %d", file, rc);
    return NULL;
error_stat:
    ufa_error_set(error, UFA_ERROR_FILE, strerror(errno));
    free(conn);
    return NULL;
error_create_table:
    ufa_error_set(error, UFA_ERROR_DATABASE, "error creating tables: %s", sqlite3_errmsg(conn->db));
    sqlite3_free(errmsg);
    sqlite3_close(conn->db);
    free(conn);
    return NULL;
}


/**
 * Gets the tag id for a tag name
 * Returns negative values on error; 0 for tag not found; id of a tag when found
 */
static int
_get_tag_id_by_name(const char *tag, ufa_error_t **error)
{
    int tag_id         = 0;
    sqlite3_stmt *stmt = NULL;
    char *sql          = "SELECT t.id FROM tag t WHERE t.name = ?";

    if (!db_prepare(&stmt, sql, error)) {
        tag_id = -1;
        goto end;
    }

    sqlite3_bind_text(stmt, 1, tag, -1, NULL);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        tag_id = sqlite3_column_int(stmt, 0);
    }
end:
    sqlite3_finalize(stmt);
    return tag_id;
}


/**
 * Retrieves all tags for a list of files, excluding the list of tags passed
 * as argument.
 */
static ufa_list_t *
_get_tags_for_files_excluding(ufa_list_t *file_ids, ufa_list_t *tags, ufa_error_t **error)
{
    if (file_ids == NULL) {
        return NULL;
    }

    char *sql           = "SELECT DISTINCT t.name FROM tag t, file_tag ft, file f WHERE ft.id_tag "
                          " = t.id AND f.id = ft.id_file AND f.id IN (%s) AND t.name NOT IN (%s)";
    ufa_list_t *result  = NULL;
    char *sql_file_args = _sql_arg_list(file_ids);
    char *sql_tags_args = _sql_arg_list(tags);
    char *full_sql      = ufa_str_sprintf(sql, sql_file_args, sql_tags_args);
    ufa_debug("Query: %s", full_sql);

    sqlite3_stmt *stmt;
    if (!db_prepare(&stmt, full_sql, error)) {
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

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *tag = (const char *)sqlite3_column_text(stmt, 0);
        result = ufa_list_append(result, ufa_strdup(tag));
    }
end:
    free(sql_file_args);
    free(sql_tags_args);
    sqlite3_finalize(stmt);
    free(full_sql);
    return result;
}


static ufa_list_t *
_get_files_with_tags(ufa_list_t *tags, ufa_error_t **error)
{
    ufa_list_t *list     = NULL;
    ufa_list_t *result   = NULL;
    ufa_list_t *file_ids = NULL;
    
    int size = ufa_list_size(tags);
    if (size == 0) {
        return NULL;
    }
    char *sql_args = _sql_arg_list(tags);
    char *full_sql = ufa_str_sprintf(
        "SELECT id_file,(SELECT name FROM file WHERE id=id_file) FROM file_tag ft,tag t WHERE "
        "t.name IN (%s) AND id_tag = t.id GROUP BY id_file HAVING COUNT(id_file) = ?",
        sql_args);

    ufa_debug("Executing query: %s", full_sql);

    sqlite3_stmt *stmt;
    if (!db_prepare(&stmt, full_sql, error)) {
        goto end;
    }

    int x = 1;
    for (UFA_LIST_EACH(iter_tags, tags)) {
        sqlite3_bind_text(stmt, x++, (char *)iter_tags->data, -1, NULL);
    }
    sqlite3_bind_int(stmt, x++, size);

    int r;
    while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
        int stmt_id          = sqlite3_column_int(stmt, 0);
        int *id              = malloc(sizeof(int));
        *id                  = stmt_id;
        const char *filename = (const char *)sqlite3_column_text(stmt, 1);
        list                 = ufa_list_append(list, ufa_strdup(filename));
        file_ids             = ufa_list_append(file_ids, id);
    }

    // for the files selected get all tags that are not in list 'tags'
    // because we need to show files with and tags other than the ones in path
    ufa_list_t *other_tags = _get_tags_for_files_excluding(file_ids, tags, error);
    if (error && *error) {
        goto end;
    }

    if (list && other_tags) { // concatenate lists
        ufa_list_get_last(list)->next = other_tags;
    }
    result = list;
end:
    sqlite3_finalize(stmt);
    ufa_list_free_full(file_ids, free);
    free(sql_args);
    free(full_sql);
    return result;
}



static sqlite_int64
_insert_tag(const char *tag)
{
    sqlite_int64 id_tag = -1;
    sqlite3_stmt *stmt  = NULL;

    char *sql_insert = "INSERT INTO tag (name) values(?)";
    if (!db_prepare(&stmt, sql_insert, NULL)) {
        goto end;
    }
    sqlite3_bind_text(stmt, 1, tag, -1, NULL);
    int r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %d\n", r);
        goto end;
    }
    id_tag = sqlite3_last_insert_rowid(conn->db);
    ufa_debug("Tag inserted: %lld\n", id_tag);

end:
    sqlite3_finalize(stmt);
    return id_tag;
}


static int
_insert_file(const char *filename, ufa_error_t **error)
{
    sqlite_int64 id_file = -1;
    sqlite3_stmt *stmt   = NULL;
    char *sql_insert     = "INSERT INTO file (name) values(?)";
    if (!db_prepare(&stmt, sql_insert, error)) {
        goto end;
    }

    sqlite3_bind_text(stmt, 1, filename, -1, NULL);
    int r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %d\n", r);
        goto end;
    }
    id_file = sqlite3_last_insert_rowid(conn->db);
    ufa_debug("File inserted: %lld\n", id_file);
end:
    sqlite3_finalize(stmt);
    return id_file;
}

static int
_get_file_id_by_name(const char *filename, ufa_error_t **error)
{
    char *filepath = NULL;
    int file_id    = 0;
    char *sql      = "SELECT f.id FROM file f WHERE f.name = ?";

    sqlite3_stmt *stmt = NULL;
    if (!db_prepare(&stmt, sql, error)) {
        file_id = -1;
        goto end;
    }

    sqlite3_bind_text(stmt, 1, filename, -1, NULL);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        file_id = sqlite3_column_int(stmt, 0);
    }

    if (!file_id) {
        filepath = ufa_util_join_path(2, repository_path, filename);
        if (ufa_util_isfile(filepath)) {
            ufa_debug("File '%s' needs to be inserted on file table", filename);
            file_id = _insert_file(filename, error);
            if (file_id == -1) {
                goto end;
            }
        }
    }
end:
    free(filepath);
    sqlite3_finalize(stmt);
    return file_id;
}


static bool
_set_tag_on_file(int file_id, int tag_id, ufa_error_t **error)
{
    bool status      = false;
    char *sql_insert = "INSERT INTO file_tag (id_file, id_tag) VALUES (?, ?)";

    sqlite3_stmt *stmt = NULL;
    if (!db_prepare(&stmt, sql_insert, error)) {
        goto end;
    }

    sqlite3_bind_int(stmt, 1, file_id);
    sqlite3_bind_int(stmt, 2, tag_id);

    int r = sqlite3_step(stmt);

    if (r == SQLITE_CONSTRAINT_UNIQUE) {
        status = true;
        ufa_debug("file is already tagged");
    } else if (r == SQLITE_DONE) {
        status = true;
        ufa_debug("file tagged");
    } else {
        ufa_error_set(error, UFA_ERROR_DATABASE, "sqlite returned on insert file_tag: %d", r);
        goto end;
    }
end:
    sqlite3_finalize(stmt);
    return status;
}


/* ============================================================================================== */
/* FUNCTIONS FROM repo.h                                                                          */
/* ============================================================================================== */

/**
 * Returns the file name of a file path.
 * It returns a newly allocated string with the last part of the path
 * (separated by file separador).
 */
char *
ufa_repo_get_filename(const char *filepath)
{
    ufa_list_t *split = ufa_util_str_split(filepath, UFA_FILE_SEPARATOR);
    ufa_list_t *last  = ufa_list_get_last(split);
    char *last_part   = ufa_strdup((char *)last->data);
    ufa_list_free_full(split, free);
    return last_part;
}


bool
ufa_repo_init(const char *repository, ufa_error_t **error)
{
    if (!ufa_util_isdir(repository)) {
        ufa_error_set(error, UFA_ERROR_NOTDIR, "error: %s is not a dir", repository);
        return false;
    }
    repository_path = ufa_strdup(repository);
    char *filepath = ufa_util_join_path(2, repository_path, REPOSITORY_FILENAME);
    ufa_debug("Initializing repo %s", filepath);
    conn = _open_sqlite_conn(filepath, error);
    free(filepath);
    return true;
}


ufa_list_t *
ufa_get_all_tags(ufa_error_t **error)
{
    ufa_list_t *all_tags = NULL;
    int rows             = 0;
    int columns          = 0;
    char **result_sql    = NULL;
    char *err            = NULL;
    char *sql            = sqlite3_mprintf("SELECT name FROM tag");
    int sql_ret          = sqlite3_get_table(conn->db, sql, &result_sql, &rows, &columns, &err);

    if (sql_ret) {
        goto sqlite_error;
    }

    for (int i = columns; i < (rows + 1) * columns;) {
        // first line is column headers, so we add +1
        char *tag = ufa_strdup(result_sql[i++]);
        all_tags  = ufa_list_append(all_tags, tag);
    }

    sqlite3_free(sql);
    sqlite3_free_table(result_sql);

    return ufa_list_get_first(all_tags);

sqlite_error:
    if (all_tags) {
        ufa_list_t *head = ufa_list_get_first(all_tags);
        ufa_list_free_full(head, free);
    }
    ufa_error("Sqlite3 error: %d (%s)", sql_ret, err);
    ufa_error_set(error, UFA_ERROR_DATABASE, "Sqlite3 error: %d: %s", sql_ret, err);
    sqlite3_free(err);
    sqlite3_free(sql);
    return NULL;
}


/**
 * Checks whether a path is a tag.
 * E.g.: tag1/tag2/tag3 or tag1/tag2/file.
 */
bool
ufa_repo_is_a_tag(const char *path, ufa_error_t **error)
{
    char *last_part = ufa_repo_get_filename(path);
    bool ret = (ufa_repo_get_file_path(path, NULL) == NULL && 
                _get_tag_id_by_name(last_part, error) > 0);
    free(last_part);
    return ret;
}


char *
ufa_repo_get_file_path(const char *path, ufa_error_t **error)
{
    /* FIXME remove error? */
    char *result    = NULL;
    char *last_part = ufa_repo_get_filename(path);
    char *filepath  = ufa_str_sprintf("%s%s%s", repository_path, UFA_FILE_SEPARATOR, last_part);

    if (ufa_util_isfile(filepath)) {
        result = filepath;
    } else {
        free(filepath);
        result = NULL;
    }

    free(last_part);
    return result;
}


ufa_list_t *
ufa_repo_list_files_for_dir(const char *path, ufa_error_t** error)
{
    ufa_list_t *list = NULL;
    if (ufa_util_strequals(path, "/")) {
        /* FIXME cache. it is better to clone all_tags */
        return ufa_get_all_tags(error);
    } else {
        ufa_list_t *list_of_tags = ufa_util_str_split(path, "/");
        // get all files with tags
        list = _get_files_with_tags(list_of_tags, error);
        ufa_list_free_full(list_of_tags, free);
    }
    return list;
}


/* TODO use error argument and returns the list */
bool
ufa_repo_get_tags_for_file(const char *filepath, ufa_list_t **list, ufa_error_t **error)
{
    bool status = true;

    /* TODO check if file exist */
    char *filename = ufa_repo_get_filename(filepath);
    ufa_debug("Listing tags for filename: %s (%s)", filename, filepath);
    char *sql = "SELECT DISTINCT t.name FROM file f , file_tag ft, tag t where f.id = ft.id_file "
                "and ft.id_tag = t.id and f.name=? ORDER BY t.name";

    sqlite3_stmt *stmt = NULL;
    if (!db_prepare(&stmt, sql, error)) {
        status = false;
        goto end;
    }
    
    sqlite3_bind_text(stmt, 1, filename, -1, NULL);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *tag = (const char *)sqlite3_column_text(stmt, 0);
        *list = ufa_list_append(*list, ufa_strdup(tag));
    }
end:
    sqlite3_finalize(stmt);
    free(filename);
    return status;
}

/**
 * Negative values on error 
 */
int
ufa_repo_insert_tag(const char *tag, ufa_error_t **error)
{
    int tag_id         = _get_tag_id_by_name(tag, error);
    sqlite3_stmt *stmt = NULL;

    if (tag_id == 0) {
        ufa_debug("tag '%s' does not exist. Inserting tag ...\n", tag);
        tag_id = _insert_tag(tag);
    } else if (tag_id > 0) {
        ufa_debug("Tag '%s' already exist\n", tag);
    }

    sqlite3_finalize(stmt);
    return tag_id;
}


bool
ufa_repo_set_tag_on_file(const char *filepath, const char *tag, ufa_error_t **error)
{
    ufa_debug("Setting tag '%s' for file '%s'", tag, filepath);

    bool status    = false;
    char *filename = ufa_repo_get_filename(filepath);

    int tag_id = ufa_repo_insert_tag(tag, error);
    if (tag_id < 0) {
        goto end;
    }

    int file_id = _get_file_id_by_name(filename, error);
    if (file_id < 0) {
        goto end;
    } else if (file_id == 0) {
        ufa_error_set(error, UFA_ERROR_STATE, "file '%s' does not exist", filepath);
        goto end;
    }

    status = _set_tag_on_file(file_id, tag_id, error);
end:
    free(filename);
    return status;
}


bool
ufa_repo_clear_tags_for_file(const char *filepath, ufa_error_t **error)
{
    int r              = 0;
    bool status        = false;
    sqlite3_stmt *stmt = NULL;
    char *sql          = "DELETE FROM file_tag WHERE id_file=?";

    int file_id = _get_file_id_by_name(filepath, error);
    if (file_id < 0) {
        goto end;
    } else if (file_id == 0) {
        ufa_error_set(error, UFA_ERROR_STATE, "file '%s' does not exist", filepath);
        goto end;
    }
    
    if (!db_prepare(&stmt, sql, NULL)) {
        goto end;
    }

    sqlite3_bind_int(stmt, 1, file_id);

    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %d\n", r);
        goto end;
    }
    status = true;
end:
    sqlite3_finalize(stmt);
    return status;
}


bool
ufa_repo_unset_tag_on_file(const char *filepath, const char *tag, ufa_error_t **error)
{
    bool status        = 0;
    sqlite3_stmt *stmt = NULL;
    char *sql_delete   = "DELETE FROM file_tag WHERE id_file = ? AND id_tag = (SELECT t.id FROM tag"
                         " t WHERE t.name = ?)";

    int file_id = _get_file_id_by_name(filepath, error);
    if (file_id == -1) {
        fprintf(stderr, "file '%s' does not exist\n", filepath);
        goto end;
    }
    
    if (!db_prepare(&stmt, sql_delete, error)) {
        goto end;
    }

    sqlite3_bind_int(stmt, 1, file_id);
    sqlite3_bind_text(stmt, 2, tag, -1, NULL);

    if (!db_execute(stmt, error)) {
        goto end;
    }

    status = true;
end:
    sqlite3_finalize(stmt);
    return status;
}