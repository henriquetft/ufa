/*
* Copyright (c) 2022 Henrique Teófilo
* All rights reserved.
*
* Implementation of monitor module (monitor.h) using inotify.
*
* For the terms of usage and distribution, please see COPYING file.
*/

#include "core/monitor.h"
#include "util/hashtable.h"
#include "util/logging.h"
#include "util/misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>


/* ========================================================================== */
/* VARIABLES AND DEFINITIONS                                                  */
/* ========================================================================== */

#define   EVENT_SIZE     (sizeof (struct inotify_event))

#define   BUF_LEN        (1024 * (EVENT_SIZE + 16))


static int inotify                      = -1;
static ufa_hashtable_t *table           = NULL;
static ufa_hashtable_t *callbacks       = NULL;
static ufa_hashtable_t *buffered_events = NULL;
static pthread_t events_loop_thread;


/* ========================================================================== */
/* AUXILIARY FUNCTIONS                                                        */
/* ========================================================================== */


static int *int_dup(int i)
{
	int* n = malloc(sizeof(int));
	*n = i;
	return n;
}

static int int_equals(int *a, int *b)
{
	return *a == *b;
}

static int int_hash(int *i)
{
	return *i;
}

static int *uint32_dup(uint32_t i)
{
	uint32_t* n = malloc(sizeof(uint32_t));
	*n = i;
	return n;
}

static int uint32_equals(uint32_t *a, uint32_t *b)
{
	return *a == *b;
}

static int uint32_hash(uint32_t *i)
{
	return *i;
}


static struct ufa_event *new_ufa_event(enum ufa_monitor_event event_type,
				       int watcher1,
				       int watcher2,
				       const char *target1,
				       const char *target2)
{
	struct ufa_event *event = malloc(sizeof *event);
	event->event    = event_type;
	event->watcher1 = watcher1;
	event->watcher2 = watcher2;
	event->target1  = target1;
	event->target2  = target2;
	return event;
}

static struct ufa_event *free_ufa_event(struct ufa_event *uevent)
{
	free(uevent->target1);
	free(uevent->target2);
	free(uevent);
}


static int to_inotify_mask(enum ufa_monitor_event events)
{
	int mask = 0;
	if (events & UFA_MONITOR_MOVE) {
		mask |= IN_MOVE;
	}
	if (events & UFA_MONITOR_DELETE) {
		mask |= IN_DELETE;
	}
	if (events & UFA_MONITOR_CLOSEWRITE) {
		mask |= IN_CLOSE_WRITE;
	}

	return mask;
}


static void mask_to_str(unsigned int mask, char *str)
{
	strcpy(str, "");
	int masks[] = { IN_ACCESS, IN_ATTRIB, IN_CLOSE_WRITE, IN_CLOSE_NOWRITE,
		       IN_CREATE, IN_DELETE, IN_DELETE_SELF, IN_MODIFY,
		       IN_MOVE_SELF, IN_MOVED_FROM, IN_MOVED_TO, IN_OPEN };

	char* strs[] = { "IN_ACCESS", "IN_ATTRIB", "IN_CLOSE_WRITE", "IN_CLOSE_NOWRITE",
			"IN_CREATE", "IN_DELETE", "IN_DELETE_SELF", "IN_MODIFY",
			"IN_MOVE_SELF", "IN_MOVED_FROM", "IN_MOVED_TO", "IN_OPEN" };
	int len = 12;

	int x;
	for (x = 0; x < len; x++) {
		if (mask & masks[x]) {
			strcat(str, strs[x]);
			strcat(str, " ");
		}
	}
}

static void print_event(char *str, const struct inotify_event *event)
{
	mask_to_str(event->mask, str);
	printf("\n------------------------------------\n");
	printf ("wd=%d mask=%u cookie=%u len=%u\n mask_str=%s\n",
	       event->wd, event->mask,
	       event->cookie, event->len, str);
	if (event->len) {
		printf ("name=%s\n", event->name);
	}
	printf("------------------------------------\n");
}


static void process_ufa_event(struct ufa_event *uevent)
{
	int watcher = 0;
	if (uevent->watcher1) {
		watcher = uevent->watcher1;
	} else if (uevent->watcher2) {
		watcher = uevent->watcher2;
	}
	// TODO invoke callback for 2 watchers if they are different?

	// invoke callback
	ufa_monitor_callback_t func = ufa_hashtable_get(callbacks, &watcher);
	if (func != NULL) {
		ufa_debug("Invoking callback for %d: %p\n", watcher, func);
		func(uevent);
		ufa_debug("Callback exited");
	}

	free_ufa_event(uevent);
}


static void handle_inotify_delete(struct inotify_event *event)
{
	ufa_debug("Delete:");
	struct ufa_event *uevent = NULL;

	char *dir = ufa_hashtable_get(table, &(event->wd));
	char *filepath = ufa_util_joinpath(dir, event->name, NULL);

	uevent = new_ufa_event(UFA_MONITOR_DELETE,
			       event->wd, 0,
			       filepath, NULL);

	process_ufa_event(uevent);
}

static void handle_inotify_closewrite(struct inotify_event *event)
{
	ufa_debug("Close write event");
	struct ufa_event *uevent = NULL;

	char *dir = ufa_hashtable_get(table, &(event->wd));
	char *filepath = ufa_util_joinpath(dir, event->name, NULL);

	uevent = new_ufa_event(UFA_MONITOR_CLOSEWRITE,
			       event->wd, 0,
			       filepath, NULL);

	process_ufa_event(uevent);
}

static void handle_inotify_moved(const struct inotify_event *event_from,
				 const struct inotify_event *event_to)
{
	struct ufa_event *uevent = NULL;

	if (event_to && event_from) {
		ufa_debug("\nA complete move:");
		int watcher1 = event_from->wd;
		int watcher2 = event_to->wd;

		char *dir_from = ufa_hashtable_get(table, &(event_from->wd));
		char *dir_to = ufa_hashtable_get(table, &(event_to->wd));
		char *path_from = ufa_util_joinpath(dir_from, event_from->name, NULL);
		char *path_to = ufa_util_joinpath(dir_to, event_to->name, NULL);
		ufa_debug("..Path from: %s", path_from);
		ufa_debug("..Path to: %s", path_to);
		uevent = new_ufa_event(UFA_MONITOR_MOVE,
				       watcher1, watcher2,
				       path_from, path_to);

	} else if (event_to == NULL) {
		ufa_debug("Move to outside:");
		char *dir = ufa_hashtable_get(table, &(event_from->wd));
		char *filepath = ufa_util_joinpath(dir, event_from->name, NULL);
		ufa_debug("..Moving from: %s\n", filepath);
		uevent = new_ufa_event(UFA_MONITOR_MOVE,
				       event_from->wd, 0,
				       filepath, NULL);


	} else if (event_from == NULL) {
		ufa_debug("Move from outside:");
		char *dir = ufa_hashtable_get(table, &(event_to->wd));
		char *filepath = ufa_util_joinpath(dir, event_to->name, NULL);
		ufa_debug("..Moving to: %s\n", filepath);
		uevent = new_ufa_event(UFA_MONITOR_MOVE,
				       0, event_to->wd,
				       NULL, filepath);

	}

	process_ufa_event(uevent);
}


static void read_inotify_events()
{
	ufa_debug("Reading inotify events...");

	char str[500] = "";
	char buf[BUF_LEN];
	int len, i = 0;

	len = read(inotify, buf, BUF_LEN);
	if (len < 0) {
		perror("read inotify");
	} else if (!len) {
		/* BUF_LEN too small? */
	}

	while (i < len) {
		struct inotify_event *event;

		event = (struct inotify_event *) &buf[i];
		int total_size = EVENT_SIZE + event->len;

		print_event(str, event);

		if ((event->mask & IN_MOVE) && event->cookie) {
			struct inotify_event *prev =
			    ufa_hashtable_get(buffered_events,
					      &(event->cookie));
			if (prev == NULL) {
				void *event2 = malloc(total_size);
				memcpy(event2, &buf[i], total_size);
				ufa_hashtable_put(buffered_events,
						  uint32_dup(event->cookie),
						  event2);
			} else {
				handle_inotify_moved(prev, event);
				ufa_hashtable_remove(buffered_events,
						     &(event->cookie));
			}
		} else if (event->mask & IN_DELETE) {
			handle_inotify_delete(event);
		} else if (event->mask & IN_CLOSE_WRITE) {
			handle_inotify_closewrite(event);
		}

		i += EVENT_SIZE + event->len;
	}
}

static void loop_read_events()
{
	ufa_debug("Starting event loop ...");
	while (1) {
		read_inotify_events();
		struct ufa_list *values = ufa_hashtable_values(buffered_events);
		if (values == NULL) {
			continue;
		}

		ufa_debug("Handling unpaired events ...");
		for (UFA_LIST_EACH(i, values)) {
			struct inotify_event *element = i->data;
			ufa_debug(".Unpaired event: %p\n", element);
			if (element->mask & IN_MOVED_FROM) {
				handle_inotify_moved(element, NULL);
			} else if (element->mask & IN_MOVED_TO) {
				handle_inotify_moved(NULL, element);
			}
		}

		ufa_list_free(values);
		ufa_hashtable_clear(buffered_events);
	}
}


/* ========================================================================== */
/* FUNCTIONS FROM monitor.h                                                   */
/* ========================================================================== */

bool ufa_monitor_init()
{
	int fd = inotify_init();
	if (fd < 0) {
		fprintf(stderr, "Failed to initialize inotify. Are you running "
				"a inotify-enabled kernel?\n");
		perror("inotify_init");
		return false;
	}

	table           = ufa_hashtable_new(int_hash, int_equals, free, free);
	buffered_events = ufa_hashtable_new(uint32_hash, uint32_equals, free,
					    free);
	callbacks       = ufa_hashtable_new(int_hash, int_equals, free, free);

	inotify = fd;

	// start thread
	int ret = pthread_create(&events_loop_thread,
				 NULL,
				 loop_read_events,
				 NULL);
	if (ret < 0) {
		perror("pthread_create");
	}
	return !ret;
}

int ufa_monitor_add_watcher(const char *filename, enum ufa_monitor_event events,
			    ufa_monitor_callback_t callback)
{
	int wd, mask;
	mask = to_inotify_mask(events);
	wd = inotify_add_watch(inotify, filename, mask);
	if (wd < 0) {
		goto error_inotify;
	}

	ufa_debug("Watching %d -- %s", wd, filename);
	ufa_hashtable_put(table, int_dup(wd), ufa_strdup(filename));
	ufa_hashtable_put(callbacks, int_dup(wd), callback);

	return wd;

error_inotify:
	perror("inotify_add_watch");
	return -1;
}

int ufa_monitor_remove_watcher(int watcher)
{
	int is_ok = !inotify_rm_watch(inotify, watcher);

	if (!is_ok) {
		perror("inotify_rm_watch");
	}

	ufa_hashtable_remove(table, &watcher);
	ufa_hashtable_remove(buffered_events, &watcher);
	ufa_hashtable_remove(callbacks, &watcher);

	return is_ok;
}

void ufa_event_tostr(enum ufa_monitor_event event, char *buf, size_t n)
{
	if (n <= 0) {
		return;
	}
	buf[0] = '\0';
	size_t used = 0;

	if (event & UFA_MONITOR_MOVE) {
		char *s = "MOVE ";
		strncat(buf, "MOVE ", n - used);
		used = n -used ;
	}
	if (event & UFA_MONITOR_DELETE && used < n) {
		strncat(buf, "DELETE ", n - used);
	}
	if (buf[strlen(buf)-1] == ' ') {
		buf[strlen(buf)-1] = '\0';
	}
}