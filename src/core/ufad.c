/* ========================================================================== */
/* Copyright (c) 2022-2023 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Implementation of UFA Daemon.                                              */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#include "util/daemonize.h"
#include "core/config.h"
#include "core/data.h"
#include "core/monitor.h"
#include "core/repo.h"
#include "core/errors.h"
#include "util/error.h"
#include "util/hashtable.h"
#include "util/logging.h"
#include "util/misc.h"
#include "util/string.h"
#include "json/jsonrpc_server.h"
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

static char *program_name    = "";
static char *program_version = "0.1";

/**
 * PID file path
 */
static char *PID_FILE = NULL;

/**
 * Thread that runs JSON-RPC Server
 */
static pthread_t thread_server;

/**
 * JSON-RPC Server
 */
static ufa_jsonrpc_server_t *server = NULL;

/**
 * Hash table mapping DIR -> WD
 */
static ufa_hashtable_t *table_current_dirs = NULL;

/**
 * Mask for config dir monitoring
 */
static const enum ufa_monitor_event CONFIG_DIR_MASK = UFA_MONITOR_MOVE |
						      UFA_MONITOR_DELETE |
						      UFA_MONITOR_CLOSEWRITE;
/**
 * Mask for repo monitoring
 */
static const enum ufa_monitor_event REPO_DIR_MASK = UFA_MONITOR_MOVE |
						    UFA_MONITOR_DELETE |
						    UFA_MONITOR_CLOSEWRITE;

/* ========================================================================== */
/* AUXILIARY FUNCTIONS - DECLARATION                                          */
/* ========================================================================== */

static int start_ufad(const char *program);
static void reload_config();
static void callback_event_repo(const struct ufa_event *event);
static void callback_event_config(const struct ufa_event *event);
static void *start_server(void *thread_data);

static void log_event(const struct ufa_event *event);
static void log_current_watched_dirs();
static void sig_handler(int signum);
static void print_usage(FILE *stream);

/* ========================================================================== */
/* MAIN FUNCTION                                                              */
/* ========================================================================== */

int main(int argc, char *argv[])
{
	program_name = argv[0];

	int opt;
	int log = 0;

	int exit_status       = EX_OK;
	bool foreground       = false;
	bool enablelogdetails = true;
	FILE *file_log        = NULL;
	char *filepath_log    = NULL;

	while ((opt = getopt(argc, argv, "l:FLhv")) != -1) {
		switch (opt) {
		case 'v':
			printf("%s\n", program_version);
			exit_status = EX_OK;
			goto end;
		case 'h':
			exit_status = EX_OK;
			print_usage(stdout);
			goto end;
		case 'F':
			foreground = true;
			break;
		case 'L':
			enablelogdetails = true;
			ufa_log_enablelogdetails(true);
			break;
		case 'l':
			if (log) {
				print_usage(stderr);
				exit_status = EXIT_FAILURE;
				goto end;
			} else {
				log = 1;
				enum ufa_log_level level =
				    ufa_log_level_from_str(optarg);
				ufa_log_setlevel(level);
				ufa_debug("LOG LEVEL: %s", optarg);
			}
			break;
		default:
			print_usage(stderr);
			exit_status = EXIT_FAILURE;
			goto end;
		}
	}

	// Get or create config dir
	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	if (!ufa_util_isdir(cfg_dir)) {
		struct ufa_error *error = NULL;
		ufa_util_mkdir(cfg_dir, &error);
		if (error) {
			ufa_error_exit(error, UFA_ERROR_FILE);
		}
	}

	PID_FILE = ufa_str_sprintf("%s/ufad.pid", cfg_dir);
	ufa_free(cfg_dir);

	if (!foreground) {
		ufa_log_use_syslog();
		ufa_daemon(program_name);
	}

	if (ufa_daemon_running(PID_FILE)) {
		ufa_error("ufad already running");
		exit_status = EXIT_FAILURE;
		goto end;
	}

	if (!foreground) {
		if (!log) {
			ufa_log_setlevel(UFA_LOG_INFO);
		}
		struct ufa_error *error = NULL;
		filepath_log = ufa_config_getlogfilepath(&error);
		if (error) {
			ufa_error_error(error);
		} else {
			file_log = fopen(filepath_log, "a");
			ufa_log_use_file(file_log);
		}

	}
	exit_status = start_ufad(program_name);

end:
	if (file_log) {
		fclose(file_log);
	}
	ufa_free(filepath_log);
	ufa_free(PID_FILE);
	return exit_status;
}


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */

static int start_ufad(const char *program)
{
	ufa_info("Starting %s ...", program);

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	table_current_dirs = UFA_HASHTABLE_STRING();

	char *cfg_dir = ufa_util_config_dir(CONFIG_DIR_NAME);
	ufa_info("Config dir: %s", cfg_dir);

	struct ufa_error *error = NULL;
	struct ufa_list *list_dirs_config = ufa_config_dirs(true, &error);
	if (error) {
		ufa_error_print_and_free(error);
		return EXIT_FAILURE;
	}

	if (!ufa_monitor_init()) {
		return EXIT_FAILURE;
	}

	ufa_info("Adding watcher to config dir: %s", cfg_dir);
	ufa_monitor_add_watcher(cfg_dir,
				CONFIG_DIR_MASK,
				callback_event_config);

	ufa_debug("Adding watcher to repo list_dirs_config");
	for (UFA_LIST_EACH(i, list_dirs_config)) {
		ufa_hashtable_put(table_current_dirs, ufa_str_dup(i->data),
				  NULL);
		ufa_debug("Adding watcher to: %s", i->data);
		int wd = ufa_monitor_add_watcher(i->data,
						 REPO_DIR_MASK,
						 callback_event_repo);
		if (wd < 0) {
			ufa_warn("Error watching %s", i->data);
		} else {
			bool r = ufa_hashtable_put(table_current_dirs,
						   ufa_str_dup(i->data),
						   ufa_int_dup(wd));
			assert(r == false);
		}
	}

	ufa_list_free(list_dirs_config);

	log_current_watched_dirs();

	// Start JSONRPC Server here (on another thread)
	ufa_info("Starting JSON-RPC Server ...");
	server = ufa_jsonrpc_server_new();

	int ret = pthread_create(&thread_server, NULL, start_server, NULL);
	if (ret != 0) {
		ufa_error("Error creating thread!");
		return EXIT_FAILURE;
	}

	ufa_monitor_wait();

	ufa_info("Terminating %s ...", program);

	ufa_jsonrpc_server_stop(server, NULL); // FIXME
	ufa_jsonrpc_server_free(server);

	ufa_info("%s terminated", program);

	return EXIT_SUCCESS;
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

	ufa_hashtable_t *table_new_dirs = UFA_HASHTABLE_STRING();

	// Adding new dirs to list_add
	for (UFA_LIST_EACH(i, list_dirs_config)) {
		int *wd = NULL;

		if (!ufa_hashtable_has_key(table_current_dirs, i->data)) {
			list_add =
			    ufa_list_append(list_add, ufa_str_dup(i->data));
			ufa_debug("Adding '%s' to list_add", i->data);
		} else {
			wd = ufa_hashtable_get(table_current_dirs, i->data);
			assert(wd != NULL);
			wd = ufa_intptr_dup(wd);
		}

		ufa_debug("Putting %s -> %d on table_new_dirs",
			  i->data,
			  (wd == NULL ? 0 : *wd));
		ufa_hashtable_put(table_new_dirs, ufa_str_dup(i->data), wd);
	}

	ufa_debug("Finding out dirs to remove:");
	struct ufa_list *list_current_dirs =
	    ufa_hashtable_keys(table_current_dirs);
	for (UFA_LIST_EACH(i, list_current_dirs)) {
		if (!ufa_hashtable_has_key(table_new_dirs, i->data)) {
			list_remove = ufa_list_append(list_remove,
						      ufa_str_dup(i->data));
			ufa_debug("Adding '%s' to list_remove", i->data);
		}
	}

	ufa_debug("Removing watchers appearing in list_remove:");
	for (UFA_LIST_EACH(i, list_remove)) {
		int *wd = ufa_hashtable_get(table_current_dirs, i->data);
		assert(wd != NULL);
		if (!ufa_monitor_remove_watcher(*wd)) {
			ufa_warn("Could not remove watcher: %s -> %d",
				 i->data,
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
			    table_new_dirs, ufa_str_dup(i->data),
						   ufa_int_dup(wd));
			ufa_debug("Put %s - %d on table_new_dirs", i->data, wd);
			// Assert element already exist
			assert(r == false);
		}
	}

	// Discard temporary state
	ufa_list_free_full(list_add, ufa_free);
	ufa_list_free_full(list_remove, ufa_free);
	ufa_list_free(list_dirs_config);
	ufa_list_free(list_current_dirs);

	// Discard old list and set and points it to the new one
	ufa_hashtable_free(table_current_dirs);
	table_current_dirs = table_new_dirs;

	log_current_watched_dirs();
}


/**
 * Handle events for file events on repo dir
 *
 * @param event
 */
static void callback_event_repo(const struct ufa_event *event)
{
	struct ufa_error *error = NULL;
	// FIXME handle .goutputstream-* files ?
	log_event(event);
	if (event->event == UFA_MONITOR_MOVE) {
		if (event->target1 && event->target2) {
			ufa_data_renamefile(event->target1, event->target2,
					    &error);
		} else if (event->target1) {
			ufa_data_removefile(event->target1, &error);
		} else if (event->target2) {
			// File added in folder. Do nohitng.
		} else {
			assert(false);
		}
	}

	if (error && error->code != UFA_ERROR_FILE_NOT_IN_DB) {
		ufa_error(error->message);
		ufa_error_print_and_free(error);
	}
}


/**
 * Handle a config change event (when config file is modified)
 *
 * @param event
 */
static void callback_event_config(const struct ufa_event *event)
{
	if (event->event == UFA_MONITOR_CLOSEWRITE &&
	    ufa_str_endswith(event->target1, DIRS_FILE_NAME)) {
		reload_config();
	}
}


static void *start_server(void *thread_data)
{
	struct ufa_error *error = NULL;
	server = ufa_jsonrpc_server_new();
	ufa_jsonrpc_server_start(server, &error);
	if (!server || error) {
		ufa_fatal("Error starting JSON-RPC server");
		ufa_error_print_and_free(error);
		exit(EXIT_FAILURE);
	}
	return NULL;
}


static void log_event(const struct ufa_event *event)
{
	char str[256] = "";
	ufa_event_tostr(event->event, str, 100);
	ufa_debug("========== NEW EVENT ==========");

	ufa_debug("EVENT......: %s", str);
	if (event->target1)
		ufa_debug("TARGET1....: %s", event->target1);
	if (event->target2)
		ufa_debug("TARGET2....: %s", event->target2);

	ufa_debug("WATCHER1...: %d", event->watcher1);
	ufa_debug("WATCHER2...: %d", event->watcher2);
}

static void log_current_watched_dirs()
{
	if (!table_current_dirs) {
		ufa_debug("Current dirs not initialized");
		return;
	}
	ufa_info("Currently watching %d dirs",
		 ufa_hashtable_size(table_current_dirs));
	if (ufa_log_is_logging(UFA_LOG_DEBUG)) {
		ufa_debug("Watched dirs:");
		struct ufa_list *l = ufa_hashtable_keys(table_current_dirs);
		for (UFA_LIST_EACH(i, l)) {
			int *wd =
			    ufa_hashtable_get(table_current_dirs, i->data);
			ufa_debug("   %s - %d", i->data, *wd);
		}
		ufa_list_free(l);
	}
}


static void sig_handler(int signum)
{
	if (signum == SIGINT) {
		ufa_info("SIGINT received. Shutting down...");
		ufa_monitor_stop();
	} else if (signum == SIGTERM) {
		ufa_info("SIGTERM received. Shutting down...");
		ufa_monitor_stop();
	}
}


static void print_usage(FILE *stream)
{
	fprintf(stream, "\nUsage: %s [OPTIONS] [COMMAND]\n", program_name);
	fprintf(stream, "\nUFA Daemon\n");
	fprintf(stream,
		"\nOPTIONS\n"
		"  -h\t\tPrint this help and quit\n"
		"  -v\t\tPrint version information and quit\n"
		"  -F\t\tRun in foreground\n"
		"  -l LOG_LEVEL\tLog levels: debug, info, warn, error, fatal\n"
		"\n");
}
