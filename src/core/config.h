#ifndef UFA_CONFIG_H_
#define UFA_CONFIG_H_

#include <stdbool.h>

#define    CONFIG_DIR_NAME             "ufa"
#define    DIRS_FILE_NAME              "dirs"
#define    DIRS_FILE_DEFAULT_STRING    "# UFA repository folders\n\n"

/**
 * Reads config file (DIRS_FILE_NAME) and gets all valid (existing)
 * directories listed.
 *
 * @param reload Reload from file or use the last result
 * @return List of dirs in DIRS_FILE_NAME
 */
struct ufa_list *ufa_config_dirs(bool reload);

#endif /* UFA_CONFIG_H_ */