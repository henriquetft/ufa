/* ========================================================================== */
/* Copyright (c) 2022 Henrique Teófilo                                        */
/* All rights reserved.                                                       */
/*                                                                            */
/* Definition of common error codes.                                          */
/*                                                                            */
/* This file is part of UFA Project.                                          */
/* For the terms of usage and distribution, please see COPYING file.          */
/* ========================================================================== */


#ifndef UFA_ERRORS_H_
#define UFA_ERRORS_H_

enum ufa_repo_error {
	UFA_ERROR_DATABASE       = 0,
	UFA_ERROR_NOTDIR         = 1,
	UFA_ERROR_FILE           = 2,
	UFA_ERROR_FILE_NOT_IN_DB = 3,
	UFA_ERROR_ARGS           = 4,
};

#endif /* UFA_ERRORS_H_ */
