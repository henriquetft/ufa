/* ========================================================================== */
/* Copyright (c) 2022-2024 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for UFA configuration mechanism.                               */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_CONFIG_H_
#define UFA_CONFIG_H_

#include "util/error.h"
#include <stdbool.h>

#define CONFIG_DIR_NAME             "ufa"
#define DIRS_FILE_NAME              "dirs"
#define LOG_FILE_NAME               "ufad.log"
#define DIRS_FILE_DEFAULT_STRING    "# UFA repository folders\n\n"


/**
 * Read config file (DIRS_FILE_NAME) and get all valid (existing)
 * listed directories.
 *
 * @param reload Reload from file or use the last result
 * @param error pointer to pointer to error structure
 * @return A newly-allocated list of dirs in DIRS_FILE_NAME
 * (should be freed using ufa_list_free)
 */
struct ufa_list *ufa_config_dirs(bool reload, struct ufa_error **error);


/**
 * Add directory to watch list
 *
 * @param dir New directory to watch
 * @param error Pointer to pointer to error structure
 * @return true whether dir was successfully added; false otherwise
 */
bool ufa_config_add_dir(const char *dir, struct ufa_error **error);


/**
 * Remove directory from watch list
 *
 * @param dir Directory to remove
 * @param error Pointer to pointer to error structure
 * @return true whether dir was successfully removed; false otherwise
 */
bool ufa_config_remove_dir(const char *dir, struct ufa_error **error);

/**
 * Get path of log file
 *
 * @param error pointer to pointer to error structure
 * @return A newly-allocated string containing path to log file
 */
char *ufa_config_getlogfilepath(struct ufa_error **error);


#endif /* UFA_CONFIG_H_ */