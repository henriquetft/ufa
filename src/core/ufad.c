/* ========================================================================== */
/* Copyright (c) 2022 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of UFA Daemon.                                              */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#include "core/data.h"
#include "core/repo.h"
#include "core/monitor.h"
#include "core/config.h"
#include "util/hashtable.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/error.h"
#include <stdio.h>
#include <assert.h>

/** Hash table mapping DIR -> WD */
static ufa_hashtable_t *table_current_dirs = NULL;


static const enum ufa_monitor_event CONFIG_DIR_MASK =
    UFA_MONITOR_MOVE | UFA_MONITOR_DELETE | UFA_MONITOR_CLOSEWRITE;

static const enum ufa_monitor_event REPO_DIR_MASK =
    UFA_MONITOR_MOVE | UFA_MONITOR_DELETE | UFA_MONITOR_CLOSEWRITE;


static void reload_config();

static int *intptr_dup(int *i)
{
	if (i == NULL) {
		return NULL;
	}
	int* n = malloc(sizeof(int));
	*n = *i;
	return n;
}

static int *int_dup(int i)
{
	int* n = malloc(sizeof(int));
	*n = i;
	return n;
}

static void print_event(const struct ufa_event *event)
{
	char str[100] = "";
	ufa_event_tostr(event->event, str, 100);
	printf("========== NEW EVENT ========== \n");
	if (str)
		printf("EVENT......: %s\n", str);
	if (event->target1)
		printf("TARGET1....: %s\n", event->target1);
	if (event->target2)
		printf("TARGET2....: %s\n", event->target2);

	printf("WATCHER1...: %d\n", event->watcher1);
	printf("WATCHER2...: %d\n", event->watcher2);
}


/**
 * Handle events for file events on repo dir
 *
 * @param event
 */
void callback_event_repo(const struct ufa_event *event)
{
	struct ufa_error *error = NULL;
	// handle .goutputstream-* files
	print_event(event);
	if (event->event == UFA_MONITOR_MOVE) {
		if (event->target1 && event->target2) {
			ufa_data_renamefile(event->target1, event->target2,
					    &error);
		} else if (event->target1) {
			ufa_data_removefile(event->target1, &error);
		} else if (event->target2) {
			// file added in folder. do nohitng.
		} else {
			assert(false);
		}
	}

	if (error && error->code != UFA_ERROR_FILE_NOT_IN_DB) {
		ufa_error_print_and_free(error);
	}

}


/**
 * Handle a config change event (when config file modified)
 *
 * @param event
 */
void callback_event_config(const struct ufa_event *event)
{
	if (event->event == UFA_MONITOR_CLOSEWRITE &&
	    ufa_str_endswith(event->target1, DIRS_FILE_NAME)) {
		reload_config();
	}
}

static void log_current_watched_dirs()
{
	if (!table_current_dirs) {
		ufa_debug("Current dirs not initialized");
		return;
	}
	ufa_info("Currently watching %d dirs",
		 ufa_hashtable_size(table_current_dirs));
	if (ufa_is_logging(UFA_LOG_DEBUG)) {
		ufa_debug("Watched dirs:");
		struct ufa_list *l = ufa_hashtable_keys(table_current_dirs);
		for (UFA_LIST_EACH(i, l)) {
			int *wd = ufa_hashtable_get(table_current_dirs, i->data);
			ufa_debug("   %s - %d", i->data, *wd);
		}
		ufa_list_free(l);
	}
}

static void reload_config()
{
	ufa_debug("Changing config file");
	struct ufa_error *error = NULL;
	struct ufa_list *list_add = NULL;
	struct ufa_list *list_remove = NULL;

	struct ufa_list *list_dirs_config = ufa_config_dirs(true, &error);
	if (error) {
		ufa_error_print_and_free(error);
		return;
	}

	log_current_watched_dirs();

	ufa_hashtable_t *table_new_dirs =
	    ufa_hashtable_new(ufa_str_hash, ufa_str_equals, free, free);

	// adding new dirs to list_add
	for (UFA_LIST_EACH(i, list_dirs_config)) {
		int *wd = NULL;

		if (!ufa_hashtable_has_key(table_current_dirs, i->data)) {
			list_add =
			    ufa_list_append(list_add, ufa_strdup(i->data));
			ufa_debug("Adding '%s' to list_add", i->data);
		} else {
			wd = ufa_hashtable_get(table_current_dirs, i->data);
			assert(wd != NULL);
			wd = intptr_dup(wd);
		}

		ufa_debug("Putting %s -> %d on table_new_dirs", i->data,
			  (wd == NULL ? 0 : *wd));
		ufa_hashtable_put(table_new_dirs, ufa_strdup(i->data), wd);
	}

	ufa_debug("Finding out dirs to remove:");
	struct ufa_list *list_current_dirs =
	    ufa_hashtable_keys(table_current_dirs);
	for (UFA_LIST_EACH(i, list_current_dirs)) {
		if (!ufa_hashtable_has_key(table_new_dirs, i->data)) {
			list_remove =
			    ufa_list_append(list_remove, ufa_strdup(i->data));
			ufa_debug("Adding '%s' to list_remove", i->data);
		}
	}

	ufa_debug("Removing watchers appearing in list_remove:");
	for (UFA_LIST_EACH(i, list_remove)) {
		int *wd = ufa_hashtable_get(table_current_dirs, i->data);
		assert(wd != NULL);
		if (!ufa_monitor_remove_watcher(*wd)) {
			ufa_warn("Could not remove watcher: %s -> %d", i->data,
				 *wd);
		}
	}

	ufa_debug("Adding watchers appearing in list_add");
	for (UFA_LIST_EACH(i, list_add)) {
		int wd = ufa_monitor_add_watcher(i->data, REPO_DIR_MASK,
						 callback_event_repo);
		if (wd < 0) {
			ufa_warn("Error adding watch. for %s", i->data);
			bool r = ufa_hashtable_remove(table_new_dirs, i->data);
			assert(r == true);
		} else {
			bool r = ufa_hashtable_put(
			    table_new_dirs, ufa_strdup(i->data), int_dup(wd));
			ufa_debug("Put %s - %d on table_new_dirs", i->data, wd);
			// assert element already exist
			assert(r == false);
		}
	}

	// discarding temporary state
	ufa_list_free_full(list_add, free);
	ufa_list_free_full(list_remove, free);
	ufa_list_free_full(list_dirs_config, free);
	ufa_list_free(list_current_dirs);

	// discard old list and set and points it to the new one
	ufa_hashtable_free(table_current_dirs);
	table_current_dirs = table_new_dirs;

	log_current_watched_dirs();
}




int main(int argc, char *argv[])
{
	// FIXME daemonize process
	table_current_dirs =
	    ufa_hashtable_new(ufa_str_hash, ufa_str_equals, free, free);

	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	ufa_info("Config dir: %s\n", cfg_dir);

	struct ufa_error *error = NULL;
	struct ufa_list *list_dirs_config = ufa_config_dirs(true, &error);
	if (error) {
		ufa_error_print_and_free(error);
		exit(-1);
	}

	if (!ufa_monitor_init()) {
		exit(-1);
	}

	ufa_debug("Adding watcher to config dir: %s", cfg_dir);
	ufa_monitor_add_watcher(cfg_dir, CONFIG_DIR_MASK,
				callback_event_config);

	ufa_debug("Adding watcher to repo list_dirs_config");
	for (UFA_LIST_EACH(i, list_dirs_config)) {
		ufa_hashtable_put(table_current_dirs, ufa_strdup(i->data),
				  NULL);
		ufa_debug("Adding watcher to: %s", i->data);
		int wd = ufa_monitor_add_watcher(i->data, REPO_DIR_MASK,
						 callback_event_repo);
		if (wd < 0) {
			ufa_warn("Error watching %s", i->data);
		} else {
			bool r =
			    ufa_hashtable_put(table_current_dirs,
					      ufa_strdup(i->data), int_dup(wd));
			assert(r == false);
		}
	}

	ufa_list_free_full(list_dirs_config, free);

	log_current_watched_dirs();

	ufa_monitor_wait();

	printf("Terminating %s\n", argv[0]);
	return 0;
}