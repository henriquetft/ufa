/* ========================================================================== */
/* Copyright (c) 2022-2023 Henrique Teófilo                                   */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definitions for monitor module.                                            */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_MONITOR_H_
#define UFA_MONITOR_H_

#include <stdbool.h>
#include <stdlib.h>

enum ufa_monitor_event {
	UFA_MONITOR_MOVE       = 1,
	UFA_MONITOR_DELETE     = 2,
	UFA_MONITOR_CLOSEWRITE = 4
};

struct ufa_event {
	enum ufa_monitor_event event;
	int watcher1;
	int watcher2;
	char *target1;
	char *target2;
};

typedef void (*ufa_monitor_event_fn_t)(const struct ufa_event *);


bool ufa_monitor_init();

bool ufa_monitor_stop();

bool ufa_monitor_wait();

int ufa_monitor_add_watcher(const char *filepath,
			    enum ufa_monitor_event events,
			    ufa_monitor_event_fn_t callback);

bool ufa_monitor_remove_watcher(int watcher);

void ufa_event_tostr(enum ufa_monitor_event event, char *buf, size_t n);

#endif /* UFA_MONITOR_H_ */