/**
 * @file docutil.c
 * @brief Implementation of document utility functions
 *
 * This module implements utility functions for document operations,
 * including path management and keyword searching.
 */

#include "docutil.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "storage.h"
#include "docroot.h"
#include "document.h"
#include "logger.h"

/* === Constants ================================================== */

/** Buffer size for file reading operations */
#define BUF_SZ 8192

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

/* === Path Management =========================================== */

/**
 * @brief Build full path for a document
 *
 * @param key Document key
 * @param dst Buffer to store the full path
 * @param dst_cap Capacity of the destination buffer
 * @return int OS_OK on success, OS_ERROR on failure
 */
int
doc_build_path(int key, char *dst, size_t dst_cap)
{
    if (!dst || dst_cap == 0) {
        util_error("Invalid destination buffer\n");
        return OS_ERROR;
    }

    const char *root = docroot_get();
    if (!root) {
        util_error("Document root not set\n");
        return OS_ERROR;
    }

    document_t doc;
    if (stg_get_doc(key, &doc) == OS_ERROR) {
        util_error("Failed to get document %d\n", key);
        return OS_ERROR;
    }

    if (doc.key == -1) {
        util_error("Document %d is deleted\n", key);
        return OS_ERROR;
    }

    int n = snprintf(dst, dst_cap, "%s/%s", root, doc.path);
    if (n < 0 || (size_t)n >= dst_cap) {
        util_error("Path too long for document %d\n", key);
        return OS_ERROR;
    }

    return OS_OK;
}

/* === Keyword Search ============================================ */

/**
 * @brief Count keyword occurrences in a file
 *
 * @param path Path to the file to search
 * @param kw Keyword to search for
 * @param stop_at_first Whether to stop at first match
 * @param out Pointer to store the count
 * @return int OS_OK on success, OS_ERROR on failure
 */
int
doc_count_keyword(const char *path,
                  const char *kw,
                  int stop_at_first,
                  size_t *out)
{
    if (!path || !kw || !out) {
        util_error("Invalid arguments to doc_count_keyword\n");
        return OS_ERROR;
    }

    *out = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        util_error("Failed to open file %s: %s\n", path, strerror(errno));
        return OS_ERROR;
    }

    size_t kw_len = strlen(kw);
    if (kw_len == 0) {
        close(fd);
        return OS_OK;
    }

    char buf[BUF_SZ];
    size_t match = 0;
    int matched_line = 0;

    ssize_t n;
    while ((n = read(fd, buf, BUF_SZ)) > 0)
    {
        for (ssize_t i = 0; i < n; ++i)
        {
            char c = buf[i];
            if (c == kw[match])
            {
                if (++match == kw_len)
                {
                    matched_line = 1;
                    match = 0;
                    if (stop_at_first) {
                        *out = 1;
                        close(fd);
                        return OS_OK;
                    }
                }
            } else {
                match = (c == kw[0]) ? 1 : 0;
            }

            if (c == '\n') {
                if (matched_line) ++(*out);
                matched_line = 0;
            }
        }
    }

    if (match > 0 && matched_line) ++(*out); /* last line no newline */

    int rc = (n < 0) ? OS_ERROR : OS_OK;
    if (rc == OS_ERROR) {
        util_error("Error reading file %s: %s\n", path, strerror(errno));
    }

    close(fd);
    return rc;
}

/**
 * @brief Check if a file contains a keyword
 *
 * @param path Path to the file to search
 * @param kw Keyword to search for
 * @return int 1 if found, 0 if not found, OS_ERROR on failure
 */
int
doc_contains_keyword(const char *path, const char *kw)
{
    size_t dummy;
    int rc = doc_count_keyword(path, kw, 1, &dummy);
    if (rc == OS_OK) return (int)dummy;
    return rc;
}

/**
 * @brief Check if a document contains a keyword
 *
 * @param key Document key
 * @param kw Keyword to search for
 * @return int 1 if found, 0 if not found, OS_ERROR on failure
 */
int
doc_key_contains_keyword(int key, const char *kw)
{
    char fullpath[PATH_MAX];
    if (doc_build_path(key, fullpath, sizeof fullpath) == OS_ERROR) {
        return OS_ERROR;
    }

    return doc_contains_keyword(fullpath, kw);
}