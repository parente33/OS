/**
 * @file docutil.h
 * @brief Document utility functions
 *
 * This module provides utility functions for document operations,
 * including path management and keyword searching.
 */

#ifndef DOCUTIL_H
#define DOCUTIL_H

#include <stddef.h>
#include <stdint.h>
#include "status.h"

/* === Path Management =========================================== */

/**
 * @brief Build full path for a document
 *
 * Constructs the full filesystem path for a document by combining
 * the document root with the document's relative path.
 *
 * @param key Document key
 * @param dst Buffer to store the full path
 * @param dst_cap Capacity of the destination buffer
 * @return int OS_OK on success, OS_ERROR on failure
 */
int doc_build_path(int key, char *dst, size_t dst_cap);

/* === Keyword Search ============================================ */

/**
 * @brief Count keyword occurrences in a file
 *
 * Searches a file for a keyword and counts the number of lines
 * containing the keyword. Can optionally stop at the first match.
 *
 * @param path Path to the file to search
 * @param kw Keyword to search for
 * @param stop_at_first Whether to stop at first match
 * @param out Pointer to store the count
 * @return int OS_OK on success, OS_ERROR on failure
 */
int doc_count_keyword(const char *path, const char *kw,
                     int stop_at_first, size_t *out);

/**
 * @brief Check if a file contains a keyword
 *
 * Searches a file for a keyword and returns whether it was found.
 *
 * @param path Path to the file to search
 * @param kw Keyword to search for
 * @return int 1 if found, 0 if not found, OS_ERROR on failure
 */
int doc_contains_keyword(const char *path, const char *kw);

/**
 * @brief Check if a document contains a keyword
 *
 * Searches a document by key for a keyword and returns whether
 * it was found.
 *
 * @param key Document key
 * @param kw Keyword to search for
 * @return int 1 if found, 0 if not found, OS_ERROR on failure
 */
int doc_key_contains_keyword(int key, const char *kw);

#endif /* DOCUTIL_H */