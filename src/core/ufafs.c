/*
 * Copyright (c) 2021 Henrique Teófilo
 * All rights reserved.
 *
 * Implementation of UFAFS -- a FUSE file system.
 *
 * For the terms of usage and distribution, please see COPYING file.
 */

#define FUSE_USE_VERSION 31
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "list.h"
#include "logging.h"
#include "misc.h"
#include "repo.h"


static struct options {
    const char *repository;
    int show_help;
} options;

#define OPTION(t, p)                                                                               \
    {                                                                                              \
        t, offsetof(struct options, p), 1                                                          \
    }
static const struct fuse_opt option_spec[] = { OPTION("--repository=%s", repository),
    OPTION("-h", show_help), OPTION("--help", show_help), FUSE_OPT_END };


static void
show_help(const char *progname)
{
    printf("usage: %s [options] <mountpoint>\n\n", progname);
    printf("File-system specific options:\n"
           "    --repository=<s>          Folder containing files and metadata\n"
           "\n");
}

static void *
ufa_fuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    cfg->kernel_cache = 1;
    return NULL;
}

static int
ufa_fuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    ufa_debug("getattr: '%s'", path);
    char *filepath = NULL;
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    /* IS ROOT */
    if (ufa_util_strequals(path, "/")) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;

        /* it is a file */
    } else if ((filepath = ufa_repo_get_file_path(path)) != NULL) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;

        struct stat st;
        stat(filepath, &st);
        __off_t size = st.st_size;
        stbuf->st_size = size;

        /* it is another tag */;
    } else if (ufa_repo_is_a_tag(path)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        res = -ENOENT;
    }

    if (filepath) {
        free(filepath);
    }
    return res;
}

static int
ufa_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
    struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    ufa_debug("readdir: '%s'", path);
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    ufa_list_t *list = ufa_repo_list_files_for_dir(path);
    ufa_list_t *head = list;

    for (; (list != NULL); list = list->next) {
        ufa_debug("...listing '%s'", (char *)list->data);
        filler(buf, list->data, NULL, 0, 0);
    }

    if (head) {
        ufa_list_free_full(head, free);
    } else {
        return -ENOENT;
    }

    return 0;
}

static int
ufa_fuse_open(const char *path, struct fuse_file_info *fi)
{
    ufa_debug("open: '%s' ---> '%s'\n", path, ufa_repo_get_file_path(path));

    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        printf("ok\n");
        return -EACCES;
    }

    // it is allowed to open
    return 0;
}

static int
ufa_fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    /* ex: /tag1/arquivo_real.txt */
    char *filepath = ufa_repo_get_file_path(path);
    ufa_debug("read: %s ---> %s (%ld / %lu)", path, filepath, offset, size);

    int res = 0;
    int f = open(filepath, O_RDONLY);

    if (f) {
        res = pread(f, buf, size, offset);
        if (res) {
            ufa_warn("Error reading file '%s': %s", filepath, strerror(errno));
        }
        close(f);
        return res;
    } else {
        ufa_warn("Error openning file '%s': %s", filepath, strerror(errno));
        return -ENOENT;
    }
}


static const struct fuse_operations ufa_fuse_oper = {
    .init = ufa_fuse_init,
    .getattr = ufa_fuse_getattr,
    .readdir = ufa_fuse_readdir,
    .open = ufa_fuse_open,
    .read = ufa_fuse_read,
};

// ./ufafs --repository=/home/henrique/files -f /home/henrique/teste
int
main(int argc, char *argv[])
{
    ufa_debug("Initializing UFA FUSE Filesystem ...");

    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1) {
        return 1;
    }

    if (options.show_help) {
        show_help(argv[0]);
        fuse_opt_add_arg(&args, "--help");
        args.argv[0][0] = '\0';
    }

    int r = ufa_repo_init(options.repository);
    if (r) {
        fprintf(stderr, "Repository '%s' is not a dir or doesn't exist\n", options.repository);
        return -1;
    }

    ret = fuse_main(args.argc, args.argv, &ufa_fuse_oper, NULL);
    printf("RET: %d\n", ret);


    ufa_debug("Exiting ...");
    fuse_opt_free_args(&args);

    return ret;
}
