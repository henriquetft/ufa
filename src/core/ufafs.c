/* ========================================================================== */
/* Copyright (c) 2021-2023 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of UFAFS -- a FUSE file system.                             */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#define FUSE_USE_VERSION 31
#include "core/repo.h"
#include "util/list.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/string.h"
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

/** Command-line options data */
static struct options {
	const char *repository;
	char *log_level;
	int show_help;
} options;

#define OPTION(t, p)                                                           \
	{                                                                      \
		t, offsetof(struct options, p), 1                              \
	}

/** File attributes about repository path */
static struct stat stat_repository;

static ufa_repo_t *repo;

/** Command-line options */
static const struct fuse_opt option_spec[] = {
    OPTION("--repository=%s", repository),
    OPTION("-h", show_help),
    OPTION("--log=%s", log_level),
    OPTION("-l %s", log_level),
    OPTION("--help", show_help),
    FUSE_OPT_END
};

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static void show_help(const char *progname);
static void copy_stat(struct stat *dest, struct stat *src);


/* ========================================================================== */
/* FUSE FUNCTIONS -- DECLARATION                                              */
/* ========================================================================== */

static void *ufa_fuse_init(struct fuse_conn_info *conn,
			   struct fuse_config *cfg);

static int ufa_fuse_getattr(const char *path,
			    struct stat *stbuf,
			    struct fuse_file_info *fi);

static int ufa_fuse_readdir(const char *path,
			    void *buf,
			    fuse_fill_dir_t filler,
			    off_t offset,
			    struct fuse_file_info *fi,
			    enum fuse_readdir_flags flags);

static int ufa_fuse_open(const char *path,
			 struct fuse_file_info *fi);

static int ufa_fuse_read(const char *path,
			 char *buf,
			 size_t size,
			 off_t offset,
			 struct fuse_file_info *fi);

static int ufa_fuse_mkdir(const char *path, mode_t mode);

/** Maps File system operations on FUSE */
static const struct fuse_operations ufa_fuse_oper = {
	.init = ufa_fuse_init,
	.getattr = ufa_fuse_getattr,
	.readdir = ufa_fuse_readdir,
	.open = ufa_fuse_open,
	.read = ufa_fuse_read,
	.mkdir = ufa_fuse_mkdir,
};


/* ========================================================================== */
/* FUSE FUNCTIONS -- IMPLEMENTATION                                           */
/* ========================================================================== */

static void *ufa_fuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	cfg->kernel_cache = 1;
	return NULL;
}

static int ufa_fuse_getattr(const char *path,
			    struct stat *stbuf,
			    struct fuse_file_info *fi)
{
	ufa_debug("getattr: '%s'", path);

	char *f = NULL;
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));

	/* THE ROOT DIR */
	if (ufa_str_equals(path, "/")) {
		copy_stat(stbuf, &stat_repository);

	/* A FILE */
	} else if ((f = ufa_repo_get_realfilepath(repo, path, NULL)) != NULL) {
		ufa_debug(".copying stat from: '%s'", f);
		struct stat st;
		stat(f, &st);
		copy_stat(stbuf, &st);

	/* ANOTHER TAG */
	} else if (ufa_repo_isatag(repo, path, NULL)) {
		copy_stat(stbuf, &stat_repository);
	} else {
		res = -ENOENT;
	}

	free(f);
	return res;
}

static int ufa_fuse_readdir(const char *path,
			    void *buf,
			    fuse_fill_dir_t filler,
			    off_t offset,
			    struct fuse_file_info *fi,
			    enum fuse_readdir_flags flags)
{
	ufa_debug("readdir: '%s'", path);

	int res = 0;
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	struct ufa_error *error = NULL;
	struct ufa_list *list = ufa_repo_listfiles(repo, path, &error);
	ufa_error_abort(error);

	for (UFA_LIST_EACH(i, list)) {
		ufa_debug("...listing '%s'", (char *) i->data);
		int r = filler(buf, i->data, NULL, 0, 0);
		if (r != 0) {
			res = -ENOMEM;
			break;
		}
	}

	if (list) {
		ufa_list_free(list);
	} else {
		res = -ENOENT;
	}

	return res;
}

static int ufa_fuse_open(const char *path,
			 struct fuse_file_info *fi)
{
	ufa_debug("open: '%s' ---> '%s'\n", path,
		  ufa_repo_get_realfilepath(repo, path, NULL));

	if ((fi->flags & O_ACCMODE) != O_RDONLY) {
		return -EACCES;
	}

	// it is allowed to open
	return 0;
}

static int ufa_fuse_read(const char *path,
			 char *buf,
			 size_t size,
			 off_t offset,
			 struct fuse_file_info *fi)
{
	/* e.g.: /tag1/real_file.txt */
	char *filepath = ufa_repo_get_realfilepath(repo, path, NULL);
	ufa_debug("read: %s ---> %s (%ld / %lu)", path, filepath, offset, size);

	int res = 0;
	int f = open(filepath, O_RDONLY);

	if (f != -1) {
		res = pread(f, buf, size, offset);
		if (res == -1) {
			ufa_warn("Error reading file '%s': %s", filepath,
				 strerror(errno));
		}
		close(f);
		return res;
	} else {
		ufa_warn("Error openning file '%s': %s",
			 filepath,
			 strerror(errno));
		return -ENOENT;
	}
}



int ufa_fuse_mkdir(const char *path, mode_t mode)
{
	ufa_debug("mkdir: %s", path);
	if (ufa_str_startswith(path, "/") && ufa_str_count(path, "/") == 1) {
		char *last_part = ufa_util_getfilename(path);
		struct ufa_error *error = NULL;
		int r = ufa_repo_inserttag(repo, last_part, &error);
		free(last_part);
		ufa_error_abort(error);
		if (r == 0) {
			return -EEXIST;
		} else if (r < 0) {
			return -ENOTDIR;
		}
		return 0;
	} else {
		return -ENOTDIR;
	}
}

// ufafs -f -s --repository=/home/henrique/files /home/henrique/teste
int main(int argc, char *argv[])
{
	ufa_debug("Initializing UFA FUSE Filesystem ...");

	if (getuid() == 0 || geteuid() == 0) {
		fprintf(stderr,
			"For security reasons you cannot run %s as root\n",
			argv[0]);
		return 1;
	}

	// initializing struct
	options.repository = NULL;
	options.log_level = NULL;
	options.show_help = 0;

	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1) {
		return 1;
	}

	if (options.show_help) {
		show_help(argv[0]);
		fuse_opt_add_arg(&args, "--help");
		args.argv[0][0] = '\0';
        	goto fuse;
	}

	if (options.repository == NULL) {
		fprintf(stderr, "Repository must be specified\n");
		return -1;
	}

	if (options.log_level) {
		enum ufa_log_level level =
		    ufa_log_level_from_str(options.log_level);
		ufa_log_setlevel(level);
	}

	struct ufa_error *error = NULL;
	repo = ufa_repo_init(options.repository, &error);
	ufa_error_print_and_free(error);
	if (repo == NULL && !options.show_help) {
		fprintf(stderr, "Could not init '%s' repo\n",
			options.repository);
		return -1;
	}
	stat(options.repository, &stat_repository);

fuse:
	ufa_debug("Calling fuse_main ...");
	ret = fuse_main(args.argc, args.argv, &ufa_fuse_oper, NULL);

	if (!options.show_help) {
		printf("fuse_main returned: %d\n", ret);
		ufa_debug("Exiting ...");
    	}

    	fuse_opt_free_args(&args);

	return ret;
}


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */


static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --repository=<s>          Folder containing files and "
	       "metadata\n"
	       "\n");
}

static void copy_stat(struct stat *dest, struct stat *src)
{
	dest->st_dev = src->st_dev;
	dest->st_ino = src->st_ino;
	dest->st_mode = src->st_mode;
	dest->st_nlink = src->st_nlink;
	dest->st_uid = src->st_uid;
	dest->st_gid = src->st_gid;
	dest->st_rdev = src->st_rdev;
	dest->st_size = src->st_size;
	dest->st_atime = src->st_atime;
	dest->st_blksize = src->st_blksize;
	dest->st_blocks = src->st_blocks;
	dest->st_ctime = src->st_ctime;
	dest->st_mtime = src->st_mtime;
}