/**
 * @file docroot.h
 * @brief Document root directory management
 *
 * This module manages the root directory for document storage,
 * providing functions to set and retrieve the document root path.
 */

#ifndef DOCROOT_H
#define DOCROOT_H

#include <stddef.h>

/* === Function Declarations ====================================== */

/**
 * @brief Set the document root directory path
 *
 * Sets the root directory where documents will be stored.
 * The path must be a valid directory path and not exceed PATH_MAX.
 *
 * @param path Directory path to set as document root
 * @return int OS_OK on success, OS_ERROR on failure
 */
int docroot_set(const char *path);

/**
 * @brief Get the current document root directory path
 *
 * Returns the currently set document root path, or NULL if
 * no path has been set.
 *
 * @return const char* Current document root path or NULL
 */
const char* docroot_get(void);

#endif /* DOCROOT_H */