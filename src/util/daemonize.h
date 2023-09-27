/* ========================================================================== */
/* Copyright (c) 2023 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Functions to create UNIX Daemons                                           */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */

#ifndef UFA_DAEMONIZE_H_
#define UFA_DAEMONIZE_H_

#include <stdbool.h>

void ufa_daemon(const char *cmd);
bool ufa_daemon_running(const char *pidfile);

#endif /* UFA_DAEMONIZE_H_ */