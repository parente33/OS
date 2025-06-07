/**
 * @file docroot.c
 * @brief Implementation of document root directory management
 *
 * This module implements the document root directory management,
 * providing a global storage location for document files.
 */

#include "docroot.h"

#include <string.h>
#include <errno.h>

#include "status.h"
#include "logger.h"

/* === Constants ================================================== */

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

/* === Internal State ============================================ */

/**
 * @brief Global document root path
 *
 * Stores the current document root directory path.
 * Empty string indicates no path has been set.
 */
static char g_doc_root[PATH_MAX] = {0};

/* === Public Functions ========================================== */

/**
 * @brief Set the document root directory path
 *
 * @param path Directory path to set as document root
 * @return int OS_OK on success, OS_ERROR on failure
 */
int
docroot_set(const char *path)
{
    if (!path) {
        util_error("Invalid document root path\n");
        return OS_ERROR;
    }

    size_t len = strlen(path);
    if (len >= PATH_MAX) {
        util_error("Document root path too long (max %d chars): %s\n",
                  PATH_MAX - 1, path);
        return OS_ERROR;
    }

    /* Copy path including null terminator */
    memcpy(g_doc_root, path, len + 1);

    return OS_OK;
}

/**
 * @brief Get the current document root directory path
 *
 * @return const char* Current document root path or NULL
 */
const char*
docroot_get(void)
{
    return g_doc_root[0] ? g_doc_root : NULL;
}