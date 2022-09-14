/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of repo module (repo.h) using sqlite3.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#include "core/repo.h"
#include "util/error.h"
#include "util/list.h"
#include "util/logging.h"
#include "util/misc.h"
#include <errno.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */
// FIXME NOT NULL FOR ATTRIBUTE TABLE
#define STR_CREATE_TABLE \
"CREATE TABLE IF NOT EXISTS \"attribute\" ( \n"\
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"id_file\"	INTEGER NOT NULL, \n"\
	"\"name\"	TEXT NOT NULL, \n"\
	"\"value\"	TEXT, \n"\
	"FOREIGN KEY(\"id_file\") REFERENCES \"file\"(\"id\") ON DELETE CASCADE\n"\
");\n"\
"CREATE TABLE IF NOT EXISTS \"file_tag\" (\n" \
	"\"id\"	INTEGER PRIMARY KEY AUTOINCREMENT, \n"\
	"\"id_file\"	INTEGER, \n"\
	"\"id_tag\"	INTEGER, \n"\
	"FOREIGN KEY(\"id_file\") REFERENCES \"file\"(\"id\") ON DELETE CASCADE, \n"\
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
"); \n"\
"CREATE UNIQUE INDEX IF NOT EXISTS \"uniq_attr\" ON \"attribute\" (\n"\
	"\"id_file\","\
	"\"name\""\
");"

#define REPOSITORY_FILENAME "repo.sqlite"
#define REPOSITORY_INDICATOR_FILE_NAME ".ufarepo"
#define db_prepare(repo, stmt, sql, error) _db_prepare(repo, stmt, sql, error, __func__)
#define db_execute(repo, stmt, error) _db_execute(repo, stmt, error, __func__)

struct ufa_repo {
	sqlite3 *db; /* sqlite3 object */
	char *name;  /* name of the file */
	char *repository_path;
};

const enum ufa_repo_matchmode ufa_repo_matchmode_supported[] = {
    UFA_REPO_EQUAL, UFA_REPO_WILDCARD};

static char *ufa_repo_matchmode_sql[] = {"=", "LIKE"};

//static struct ufa_repo *repo;

/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

bool _db_prepare(ufa_repo_t *repo, sqlite3_stmt **stmt, char *sql, struct ufa_error **error,
		 const char *func_name)
{
	assert(repo != NULL);
	int prepare_ret = sqlite3_prepare_v2(repo->db, sql, -1, stmt, NULL);
	if (prepare_ret != SQLITE_OK) {
		ufa_error_new(error, UFA_ERROR_DATABASE,
			      "error on function %s:   %s", func_name,
			      sqlite3_errmsg(repo->db));
		return false;
	}
	return true;
}

static bool _db_execute(ufa_repo_t *repo, sqlite3_stmt *stmt,
			struct ufa_error **error, const char *func_name)
{
	bool status = true;
	int r = sqlite3_step(stmt);
	if (r != SQLITE_DONE) {
		ufa_error_new(error, UFA_ERROR_DATABASE,
			      "error on function %s:   %s", __func__,
			      sqlite3_errmsg(repo->db));
		status = false;
	}
	return status;
}

/**
 * Returns a newly-allocated string with characters '?' separated by ','.
 * The number of characters '?' is equal to the number of elements in list.
 */
static char *_sql_arg_list(struct ufa_list *list)
{
	int size = ufa_list_size(list);
	if (size == 0) {
		return ufa_strdup("");
	}
	char *sql_args = ufa_str_multiply("?,", size);
	sql_args[strlen(sql_args) - 1] = '\0';
	return sql_args;
}

/**
 * Attempts to establish a connection to sqlite repository.
 * Returns the ufa_repo_conn_t or NULL on error.
 */
static struct ufa_repo *_open_sqlite_conn(const char *file,
					  const char *repo_path,
					  struct ufa_error **error)
{
	char *errmsg = NULL;
	struct ufa_repo *repo = malloc(sizeof *repo);
	int rc =
	    sqlite3_open(file, &repo->db); /* create file if it do not exist */
	sqlite3_extended_result_codes(repo->db, 1);

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
		rc = sqlite3_exec(repo->db, STR_CREATE_TABLE, NULL, 0, &errmsg);
		if (rc != SQLITE_OK) {
			goto error_create_table;
		}
	}

	repo->name = ufa_strdup(file);
	repo->repository_path = ufa_strdup(repo_path);
	return repo;

error_opening:
	free(repo);
	ufa_error_new(error, UFA_ERROR_DATABASE,
		      "Error opening SQLite db %s. Returned: %d", file, rc);
	return NULL;
error_stat:
	ufa_error_new(error, UFA_ERROR_FILE, strerror(errno));
	free(repo);
	return NULL;
error_create_table:
	ufa_error_new(error, UFA_ERROR_DATABASE, "error creating tables: %s",
		      sqlite3_errmsg(repo->db));
	sqlite3_free(errmsg);
	sqlite3_close(repo->db);
	free(repo);
	return NULL;
}

/**
 * Gets the tag id for a tag name
 * Returns negative values on error; 0 for tag not found; id of a tag when found
 */
static int _get_tag_id_by_name(ufa_repo_t *repo, const char *tag, struct ufa_error **error)
{
	int tag_id = 0;
	sqlite3_stmt *stmt = NULL;
	char *sql = "SELECT t.id FROM tag t WHERE t.name = ?";

	if (!db_prepare(repo, &stmt, sql, error)) {
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
static struct ufa_list *_get_tags_for_files_excluding(ufa_repo_t *repo,
						      struct ufa_list *file_ids,
						      struct ufa_list *tags,
						      struct ufa_error **error)
{
	if (file_ids == NULL) {
		return NULL;
	}

	char *sql = "SELECT DISTINCT t.name FROM tag t, file_tag ft, file f "
		    "WHERE ft.id_tag "
		    " = t.id AND f.id = ft.id_file AND f.id IN (%s) AND t.name "
		    "NOT IN (%s)";
	struct ufa_list *result = NULL;
	char *sql_file_args = _sql_arg_list(file_ids);
	char *sql_tags_args = _sql_arg_list(tags);
	char *full_sql = ufa_str_sprintf(sql, sql_file_args, sql_tags_args);
	ufa_debug("Query: %s", full_sql);

	sqlite3_stmt *stmt = NULL;
	if (!db_prepare(repo, &stmt, full_sql, error)) {
		goto end;
	}

	/* setting query args */
	int x = 1;
	for (struct ufa_list *list = file_ids; (list != NULL);
	     list = list->next) {
		sqlite3_bind_int(stmt, x++, *((int *)list->data));
	}

	for (struct ufa_list *list = tags; (list != NULL); list = list->next) {
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

static struct ufa_list *_get_files_with_tags(ufa_repo_t *repo,
					     struct ufa_list *tags,
					     struct ufa_error **error)
{
	struct ufa_list *list = NULL;
	struct ufa_list *result = NULL;
	struct ufa_list *file_ids = NULL;

	int size = ufa_list_size(tags);
	if (size == 0) {
		return NULL;
	}
	char *sql_args = _sql_arg_list(tags);
	char *full_sql =
	    ufa_str_sprintf("SELECT id_file,(SELECT name FROM file WHERE "
			    "id=id_file) FROM file_tag ft,tag t WHERE "
			    "t.name IN (%s) AND id_tag = t.id GROUP BY id_file "
			    "HAVING COUNT(id_file) = ?",
			    sql_args);

	ufa_debug("Executing query: %s", full_sql);

	sqlite3_stmt *stmt = NULL;
	if (!db_prepare(repo, &stmt, full_sql, error)) {
		goto end;
	}

	int x = 1;
	for (UFA_LIST_EACH(iter_tags, tags)) {
		sqlite3_bind_text(stmt, x++, (char *)iter_tags->data, -1, NULL);
	}
	sqlite3_bind_int(stmt, x++, size);

	int r;
	while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
		int stmt_id = sqlite3_column_int(stmt, 0);
		int *id = malloc(sizeof(int));
		*id = stmt_id;
		const char *filename =
		    (const char *)sqlite3_column_text(stmt, 1);
		list = ufa_list_append(list, ufa_strdup(filename));
		file_ids = ufa_list_append(file_ids, id);
	}

	// for the files selected get all tags that are not in list 'tags'
	// because we need to show files with and tags other than the ones in
	// path
	struct ufa_list *other_tags =
	    _get_tags_for_files_excluding(repo, file_ids, tags, error);
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

static sqlite_int64 _insert_tag(ufa_repo_t *repo, const char *tag)
{
	sqlite_int64 id_tag = -1;
	sqlite3_stmt *stmt = NULL;

	char *sql_insert = "INSERT INTO tag (name) values(?)";
	if (!db_prepare(repo, &stmt, sql_insert, NULL)) {
		goto end;
	}
	sqlite3_bind_text(stmt, 1, tag, -1, NULL);
	int r = sqlite3_step(stmt);
	if (r != SQLITE_DONE) {
		fprintf(stderr, "sqlite error: %d\n", r);
		goto end;
	}
	id_tag = sqlite3_last_insert_rowid(repo->db);
	ufa_debug("Tag inserted: %lld\n", id_tag);

end:
	sqlite3_finalize(stmt);
	return id_tag;
}

static int _insert_file(ufa_repo_t *repo, const char *filename,
			struct ufa_error **error)
{
	sqlite_int64 id_file = -1;
	sqlite3_stmt *stmt = NULL;
	char *sql_insert = "INSERT INTO file (name) values(?)";
	if (!db_prepare(repo, &stmt, sql_insert, error)) {
		goto end;
	}

	sqlite3_bind_text(stmt, 1, filename, -1, NULL);
	int r = sqlite3_step(stmt);
	if (r != SQLITE_DONE) {
		fprintf(stderr, "sqlite error: %d\n", r);
		goto end;
	}
	id_file = sqlite3_last_insert_rowid(repo->db);
	ufa_debug("File inserted: %lld\n", id_file);
end:
	sqlite3_finalize(stmt);
	return id_file;
}

static int _get_file_id_by_name(ufa_repo_t *repo, const char *filename,
				struct ufa_error **error)
{
	ufa_debug("%s: %s", __func__, filename);
	char *filepath = NULL;
	int file_id = 0;
	char *sql = "SELECT f.id FROM file f WHERE f.name = ?";

	sqlite3_stmt *stmt = NULL;
	if (!db_prepare(repo, &stmt, sql, error)) {
		file_id = -1;
		goto end;
	}

	sqlite3_bind_text(stmt, 1, filename, -1, NULL);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		file_id = sqlite3_column_int(stmt, 0);
	}

	if (!file_id) {
		filepath = ufa_util_joinpath(repo->repository_path, filename,
					     NULL);
		if (ufa_util_isfile(filepath)) {
			ufa_debug(
			    "File '%s' needs to be inserted on file table",
			    filename);
			file_id = _insert_file(repo, filename, error);
			if (file_id == -1) {
				ufa_error("Error inserting on db, file '%s'");
				goto end;
			}
		}
	}
end:
	free(filepath);
	sqlite3_finalize(stmt);
	return file_id;
}

static bool _set_tag_on_file(ufa_repo_t *repo, int file_id, int tag_id,
			     struct ufa_error **error)
{
	bool status = false;
	char *sql_insert =
	    "INSERT INTO file_tag (id_file, id_tag) VALUES (?, ?)";

	sqlite3_stmt *stmt = NULL;
	if (!db_prepare(repo, &stmt, sql_insert, error)) {
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
		ufa_error_new(error, UFA_ERROR_DATABASE,
			      "sqlite returned on insert file_tag: %d", r);
		goto end;
	}
end:
	sqlite3_finalize(stmt);
	return status;
}

static void _create_repo_indicator_file(char *repository)
{
	char *filepath = ufa_util_joinpath(repository,
					   REPOSITORY_INDICATOR_FILE_NAME,
					   NULL);
	if (ufa_util_isfile(filepath)) {
		goto end;
	}
	ufa_debug("Writting %s file", filepath);
	FILE *fp = fopen(filepath, "w");
	if (fp == NULL) {
		fprintf(stderr, "error openning '%s': %s\n", filepath,
			strerror(errno));
		abort();
	}
	fprintf(fp, "%s", repository);
	fclose(fp);
end:
	free(filepath);
}

int _get_file_id(ufa_repo_t *repo, const char *filepath,
		 struct ufa_error **error)
{
	char *filename = NULL;

	filename = ufa_util_getfilename(filepath);
	int file_id = _get_file_id_by_name(repo, filename, error);
	if (file_id < 0) {
		file_id = 0;
		goto end;
	} else if (file_id == 0) {
		ufa_error_new(error, UFA_ERROR_FILE_NOT_IN_DB,
			      "file '%s' does not exist in DB", filename);
	}
end:
	free(filename);
	return file_id;	
}

/* ========================================================================== */
/* FUNCTIONS FROM repo.h                                                      */
/* ========================================================================== */

ufa_repo_t *ufa_repo_init(const char *repository, struct ufa_error **error)
{
	ufa_repo_t *repo;
	if (!ufa_util_isdir(repository)) {
		ufa_error_new(error, UFA_ERROR_NOTDIR, "error: %s is not a dir",
			      repository);
		return false;
	}
	char *filepath =
	    ufa_util_joinpath(repository, REPOSITORY_FILENAME, NULL);
	ufa_debug("Initializing repo %s", filepath);
	repo = _open_sqlite_conn(filepath, repository, error);
	ufa_error_abort(*error);
	_create_repo_indicator_file(repository);
	free(filepath);
	return repo;
}

char *ufa_repo_getrepopath(ufa_repo_t *repo)
{
	ufa_return_val_ifnot(repo, NULL);
	return ufa_strdup(repo->repository_path);
}

struct ufa_list *ufa_repo_listtags(ufa_repo_t *repo, struct ufa_error **error)
{
	// FIXME check repo null
	struct ufa_list *all_tags = NULL;
	int rows = 0;
	int columns = 0;
	char **result_sql = NULL;
	char *err = NULL;
	char *sql = sqlite3_mprintf("SELECT name FROM tag");
	int sql_ret = sqlite3_get_table(repo->db, sql, &result_sql, &rows,
					&columns, &err);

	if (sql_ret) {
		goto sqlite_error;
	}

	for (int i = columns; i < (rows + 1) * columns;) {
		// first line is column headers, so we add +1
		char *tag = ufa_strdup(result_sql[i++]);
		all_tags = ufa_list_append(all_tags, tag);
	}

	sqlite3_free(sql);
	sqlite3_free_table(result_sql);

	return ufa_list_get_first(all_tags);

sqlite_error:
	if (all_tags) {
		struct ufa_list *head = ufa_list_get_first(all_tags);
		ufa_list_free_full(head, free);
	}
	ufa_error("Sqlite3 error: %d (%s)", sql_ret, err);
	ufa_error_new(error, UFA_ERROR_DATABASE, "Sqlite3 error: %d: %s",
		      sql_ret, err);
	sqlite3_free(err);
	sqlite3_free(sql);
	return NULL;
}

/**
 * Checks whether a path is a tag.
 * E.g.: /tag1/tag2/tag3 or /tag1/tag2/file.
 */
bool ufa_repo_isatag(ufa_repo_t *repo, const char *path, struct ufa_error **error)
{
	char *last_part = ufa_util_getfilename(path);
	bool ret = (ufa_repo_get_realfilepath(repo, path, NULL) == NULL &&
		    _get_tag_id_by_name(repo, last_part, error) > 0);
	free(last_part);
	return ret;
}

char *ufa_repo_get_realfilepath(ufa_repo_t *repo, const char *path,
				struct ufa_error **error)
{
	/* FIXME remove error? */
	// FIXME check if filepath is in repo?
	char *result = NULL;
	char *last_part = ufa_util_getfilename(path);

	char *filepath =
	    ufa_util_joinpath(repo->repository_path, last_part, NULL);

	if (ufa_util_isfile(filepath)) {
		result = filepath;
	} else {
		free(filepath);
		result = NULL;
	}

	free(last_part);
	return result;
}

struct ufa_list *ufa_repo_listfiles(ufa_repo_t *repo,
				    const char *dirpath,
				    struct ufa_error **error)
{
	struct ufa_list *list = NULL;
	if (ufa_str_equals(dirpath, "/")) {
		/* FIXME cache. it is better to clone all_tags */
		list = ufa_repo_listtags(repo, error);
	} else {
		struct ufa_list *list_of_tags = ufa_str_split(dirpath, "/");
		// get all files with tags
		list = _get_files_with_tags(repo, list_of_tags, error);
		ufa_list_free_full(list_of_tags, free);
	}

	list =
	    ufa_list_append(list, ufa_strdup(REPOSITORY_INDICATOR_FILE_NAME));
	return list;
}

/* TODO use error argument and returns the list */
bool ufa_repo_gettags(ufa_repo_t *repo,
		      const char *filepath,
		      struct ufa_list **list,
		      struct ufa_error **error)
{
	ufa_debug("%s: '%s'", __func__, filepath);
	bool status = false;
	char *filename = NULL;
	sqlite3_stmt *stmt = NULL;

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	filename = ufa_util_getfilename(filepath);
	ufa_debug("Listing tags for filename: %s (%s)", filename, filepath);
	char *sql = "SELECT DISTINCT t.name FROM file_tag ft, tag t "
		    "WHERE ft.id_tag = t.id AND ft.id_file=? ORDER BY t.name";
	
	if (!db_prepare(repo, &stmt, sql, error)) {
		goto end;
	}

	sqlite3_bind_int(stmt, 1, file_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *tag = (const char *)sqlite3_column_text(stmt, 0);
		*list = ufa_list_append(*list, ufa_strdup(tag));
	}
	status = true;
end:
	sqlite3_finalize(stmt);
	free(filename);
	return status;
}

/**
 * Negative values on error
 */
int ufa_repo_inserttag(ufa_repo_t *repo, const char *tag, struct ufa_error **error)
{
	int tag_id = _get_tag_id_by_name(repo, tag, error);
	sqlite3_stmt *stmt = NULL;

	if (tag_id == 0) {
		ufa_debug("tag '%s' does not exist. Inserting tag ...\n", tag);
		tag_id = _insert_tag(repo, tag);
	} else if (tag_id > 0) {
		ufa_debug("Tag '%s' already exist\n", tag);
	}

	sqlite3_finalize(stmt);
	return tag_id;
}

bool ufa_repo_settag(ufa_repo_t *repo, const char *filepath, const char *tag,
		     struct ufa_error **error)
{
	ufa_debug("Setting tag '%s' for file '%s'", tag, filepath);

	bool status = false;
	char *filename = ufa_util_getfilename(filepath);

	int tag_id = ufa_repo_inserttag(repo, tag, error);
	if (tag_id < 0) {
		goto end;
	}

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	status = _set_tag_on_file(repo, file_id, tag_id, error);
end:
	free(filename);
	return status;
}

bool ufa_repo_cleartags(ufa_repo_t *repo,
			const char *filepath,
			struct ufa_error **error)
{
	int r = 0;
	bool status = false;
	sqlite3_stmt *stmt = NULL;
	char *sql = "DELETE FROM file_tag WHERE id_file=?";

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	if (!db_prepare(repo, &stmt, sql, NULL)) {
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

bool ufa_repo_unsettag(ufa_repo_t *repo,
		       const char *filepath,
		       const char *tag,
		       struct ufa_error **error)
{
	bool status = 0;
	sqlite3_stmt *stmt = NULL;
	char *sql_delete = "DELETE FROM file_tag WHERE id_file = ? AND id_tag "
			   "= (SELECT t.id FROM tag"
			   " t WHERE t.name = ?)";

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	if (!db_prepare(repo, &stmt, sql_delete, error)) {
		goto end;
	}

	sqlite3_bind_int(stmt, 1, file_id);
	sqlite3_bind_text(stmt, 2, tag, -1, NULL);

	if (!db_execute(repo, stmt, error)) {
		goto end;
	}

	status = true;
end:
	sqlite3_finalize(stmt);
	return status;
}

struct ufa_repo_filterattr *
ufa_repo_filterattr_new(const char *attribute, const char *value,
			 enum ufa_repo_matchmode match_mode)
{
	struct ufa_repo_filterattr *ptr = calloc(1, sizeof *ptr);
	ptr->attribute = ufa_strdup(attribute);
	ptr->value = (value == NULL) ? NULL : ufa_strdup(value);
	ptr->matchmode = match_mode;
	return ptr;
}

void ufa_repo_filterattr_free(struct ufa_repo_filterattr *filter)
{
	if (filter != NULL) {
		free(filter->attribute);
		free(filter->value);
		free(filter);
	}
}

char *_generate_sql_search_attrs(struct ufa_list *filter_attr)
{
	int count_list = ufa_list_size(filter_attr);
	if (count_list == 0) {
		return ufa_strdup("");
	}
	char *new_str = malloc(count_list * 50 * sizeof *new_str);
	new_str[0] = '\0';
	const char *sql_with_value = "(a.name = ? AND a.value %s ?)";

	const char *sql_without_value = "(a.name = ?)";
	for (UFA_LIST_EACH(iter_attr, filter_attr)) {
		/* read iter_attr->next to implement properly */
		if (((struct ufa_repo_filterattr *)iter_attr->data)->value ==
		    NULL) {
			strcat(new_str, sql_without_value);
		} else {
			char *str = ufa_str_sprintf(
			    sql_with_value,
			    ufa_repo_matchmode_sql[((struct ufa_repo_filterattr
							  *)
				      iter_attr->data)
				     ->matchmode]);
			strcat(new_str, str);
			free(str);
		}

		if (iter_attr->next != NULL) {
			strcat(new_str, " OR ");
		}
	}

	char *ret = ufa_str_sprintf(
	    " AND (%s) GROUP BY f.id HAVING COUNT(f.id) = ?", new_str);
	free(new_str);
	return ret;
}

char *_generate_sql_search_tags(struct ufa_list *tags)
{
	int count_tags = ufa_list_size(tags);
	char *sql_args_tags = _sql_arg_list(tags);

	char *sql_filter_tags = NULL;

	char *fixa = "f.id IN (SELECT id_file "
		     "FROM file_tag ft,tag t "
		     "WHERE id_tag = t.id "
		     "AND t.name IN (%s) "
		     "GROUP BY id_file "
		     "HAVING COUNT(id_file) = ?) ";
	if (count_tags > 0) {
		sql_filter_tags = ufa_str_sprintf(fixa, sql_args_tags);
	} else {
		sql_filter_tags = ufa_strdup("");
	}

	free(sql_args_tags);
	return sql_filter_tags;
}

struct ufa_list *ufa_repo_search(ufa_repo_t *repo,
				 struct ufa_list *filter_attr,
				 struct ufa_list *tags,
				 struct ufa_error **error)
{
	ufa_debug("%s: %s", __func__,
		  ((struct ufa_repo *)repo)->repository_path);

	struct ufa_list *result_list_names = NULL;

	int count_tags = ufa_list_size(tags);
	int count_attrs = ufa_list_size(filter_attr);

	if (count_tags == 0 && count_attrs == 0) {
		ufa_error_new(error, UFA_ERROR_ARGS,
			      "you must search for tags or attributes");
		return NULL;
	}
	char *sql_search_tags = _generate_sql_search_tags(tags);
	char *sql_search_attrs = _generate_sql_search_attrs(filter_attr);

	char *full_sql = NULL;

	if (count_tags && !count_attrs) {
		ufa_debug("Searching by tags");
		char *sql_tags = "SELECT f.id,f.name FROM file f WHERE %s";
		full_sql = ufa_str_sprintf(sql_tags, sql_search_tags);
	} else if (!count_tags && count_attrs) {
		ufa_debug("Searching by attributes");
		char *sql_attrs = "SELECT f.id,f.name FROM file f,attribute a "
				  "WHERE a.id_file=f.id %s";
		full_sql = ufa_str_sprintf(sql_attrs, sql_search_attrs);
	} else if (count_tags && count_attrs) {
		ufa_debug("Searching by tags and attributes");
		char *sql_tags_attrs =
		    "SELECT f.id,f.name FROM file f,attribute a WHERE  %s  AND "
		    "a.id_file=f.id %s";
		full_sql = ufa_str_sprintf(sql_tags_attrs, sql_search_tags,
					   sql_search_attrs);
	}

	ufa_debug("SQL: %s", full_sql);

	sqlite3_stmt *stmt = NULL;
	if (!db_prepare(repo, &stmt, full_sql, error)) {
		goto end;
	}

	// filling tag parameters
	int x = 1;

	// filling tag parameters
	if (count_tags) {
		for (UFA_LIST_EACH(iter_tags, tags)) {
			sqlite3_bind_text(stmt, x++, (char *)iter_tags->data,
					  -1, NULL);
		}
		sqlite3_bind_int(stmt, x++, count_tags);
	}

	// filling attrs parameters
	if (count_attrs) {
		for (UFA_LIST_EACH(i, filter_attr)) {
			struct ufa_repo_filterattr *attr =
			    (struct ufa_repo_filterattr *)i->data;
			sqlite3_bind_text(stmt, x++, attr->attribute, -1, NULL);
			if (attr->value != NULL) {
				char *value = ufa_strdup(attr->value);
				if (attr->matchmode == UFA_REPO_WILDCARD) {
					ufa_str_replace(value, '*', '%');
				}
				sqlite3_bind_text(stmt, x++, value, -1,
						  SQLITE_TRANSIENT);
				free(value);
			}
		}
		sqlite3_bind_int(stmt, x++, count_attrs);
	}

	int r;
	while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
		const char *filename =
		    ufa_strdup((const char *)sqlite3_column_text(stmt, 1));
		ufa_debug("found file: %s\n", filename);
		result_list_names =
		    ufa_list_append(result_list_names, filename);
	}

end:
	sqlite3_finalize(stmt);
	free(sql_search_tags);
	free(sql_search_attrs);
	free(full_sql);
	return result_list_names;
}

bool ufa_repo_setattr(ufa_repo_t *repo,
		      const char *filepath,
		      const char *attribute,
		      const char *value,
		      struct ufa_error **error)
{
	sqlite3_stmt *stmt = NULL;
	bool status = false;
	char *sql =
	    "INSERT OR REPLACE INTO attribute(id, id_file, name, value) "
	    "VALUES((SELECT id FROM attribute WHERE id_file=? AND name=?), ?, "
	    "?, ?)";

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	ufa_debug("SQL for function '%s': %s", __func__, sql);

	if (!db_prepare(repo, &stmt, sql, error)) {
		goto end;
	}

	sqlite3_bind_int(stmt, 1, file_id);
	sqlite3_bind_text(stmt, 2, attribute, -1, NULL);
	sqlite3_bind_int(stmt, 3, file_id);
	sqlite3_bind_text(stmt, 4, attribute, -1, NULL);
	sqlite3_bind_text(stmt, 5, value, -1, NULL);

	if (!db_execute(repo, stmt, error)) {
		goto end;
	}

	status = true;

end:
	sqlite3_finalize(stmt);
	return status;
}



bool ufa_repo_unsetattr(ufa_repo_t *repo,
			const char *filepath,
			const char *attribute,
			 struct ufa_error **error)
{
	sqlite3_stmt *stmt = NULL;
	bool status = false;
	char *sql = "DELETE from attribute WHERE id_file=? AND name=?";

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	ufa_debug("SQL for function '%s': %s", __func__, sql);

	if (!db_prepare(repo, &stmt, sql, error)) {
		goto end;
	}

	sqlite3_bind_int(stmt, 1, file_id);
	sqlite3_bind_text(stmt, 2, attribute, -1, NULL);

	if (!db_execute(repo, stmt, error)) {
		goto end;
	}

	status = true;

end:
	sqlite3_finalize(stmt);
	return status;
}

struct ufa_list *ufa_repo_getattr(ufa_repo_t *repo,
				  const char *filepath,
				  struct ufa_error **error)
{
	struct ufa_list *result_list_attrs = NULL;
	sqlite3_stmt *stmt = NULL;
	char *sql = "SELECT name,value FROM attribute WHERE id_file=?";

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	ufa_debug("SQL for function '%s': %s", __func__, sql);

	if (!db_prepare(repo, &stmt, sql, error)) {
		goto end;
	}

	sqlite3_bind_int(stmt, 1, file_id);

	int r;
	while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
		const char *attribute =
		    ufa_strdup((const char *)sqlite3_column_text(stmt, 0));
		const char *value =
		    ufa_strdup((const char *)sqlite3_column_text(stmt, 1));
		struct ufa_repo_attr *attr = calloc(1, sizeof *attr);
		attr->attribute = attribute;
		attr->value = value;
		result_list_attrs = ufa_list_append(result_list_attrs, attr);
	}

end:
	sqlite3_finalize(stmt);
	return result_list_attrs;
}


void ufa_repo_free(ufa_repo_t *repo)
{
	if (repo != NULL) {
		sqlite3_close(repo->db);
		free(repo->name);
		free(repo->repository_path);
		free(repo);
	}
}

char *ufa_repo_getrepofolderfor(const char *filepath, struct ufa_error **error)
{
	FILE *file_read = NULL;
	char *dirname = NULL;
	char *repodb_file = NULL;
	char *repo_ind_file = NULL;
	char *repofile = NULL;

	char *repository = NULL;


	if (ufa_util_isdir(filepath)) {
		dirname = ufa_strdup(filepath);
	} else if (ufa_util_isfile(filepath)) {
		dirname = ufa_util_dirname(filepath);
	} else {
		ufa_error_new(error, UFA_ERROR_FILE,
			      "%s is not a file", filepath);
		goto end;
	}



	repodb_file = ufa_util_joinpath(dirname, REPOSITORY_FILENAME, NULL);

	// use .db
	if (ufa_util_isfile(repodb_file)) {
		repository = ufa_strdup(dirname);
	} else {
		repo_ind_file = ufa_util_joinpath(
		    dirname, REPOSITORY_INDICATOR_FILE_NAME, NULL);

		if (ufa_util_isfile(repo_ind_file)) {
			file_read = fopen(repo_ind_file, "r");
			char linebuf[1024];
			if (fgets(linebuf, 1024, file_read) != NULL) {
				char *line = ufa_str_trim(linebuf);
				repository = ufa_strdup(line);
			} else {
				perror("fgets");
				goto end;
			}
		}
	}

	if (repository == NULL) {
		ufa_error_new(error, UFA_ERROR_FILE,
			      "not found repo for: %s", filepath);
		goto end;
	}

end:
	if (file_read != NULL) {
		fclose(file_read);
	}
	free(repofile);
	free(dirname);
	free(repodb_file);
	free(repo_ind_file);
	return repository;
}




void ufa_repo_attrfree(struct ufa_repo_attr *attr)
{
	if (attr) {
		free(attr->attribute);
		free(attr->value);
		free(attr);
	}
}

bool ufa_repo_isrepo(char *directory)
{
	if (!ufa_util_isdir(directory)) {
		return false;
	}
	char *fpath = ufa_util_joinpath(directory, REPOSITORY_FILENAME, NULL);

	bool ret = ufa_util_isfile(fpath);

	ufa_free(fpath);

	return ret;
}

bool ufa_repo_removefile(ufa_repo_t *repo, char *filepath,
			 struct ufa_error **error)
{
	sqlite3_stmt *stmt = NULL;
	bool status = false;
	char *sql = "DELETE FROM file WHERE id=?";

	int file_id;
	if (!(file_id = _get_file_id(repo, filepath, error))) {
		goto end;
	}

	if (!db_prepare(repo, &stmt, sql, error)) {
		goto end;
	}

	sqlite3_bind_int(stmt, 1, file_id);

	if (!db_execute(repo, stmt, error)) {
		goto end;
	}

	int affected = sqlite3_changes(repo->db);
	status = (affected == 1);
end:
	return status;
}


bool ufa_repo_renamefile(ufa_repo_t *repo, char *oldfilepath, char *newfilepath,
			 struct ufa_error **error)
{
	sqlite3_stmt *stmt = NULL;
	char *new_filename = NULL;
	bool status = false;
	char *sql = "UPDATE file SET name=? WHERE id=?";

	int file_id;
	if (!(file_id = _get_file_id(repo, oldfilepath, error))) {
		goto end;
	}

	if (!db_prepare(repo, &stmt, sql, error)) {
		goto end;
	}

	new_filename = ufa_util_getfilename(newfilepath);
	sqlite3_bind_text(stmt, 1, new_filename, -1, NULL);
	sqlite3_bind_int(stmt, 2, file_id);

	if (!db_execute(repo, stmt, error)) {
		goto end;
	}

	int affected = sqlite3_changes(repo->db);
	status = (affected == 1);

end:
	ufa_free(new_filename);
	sqlite3_finalize(stmt);
	return status;
}