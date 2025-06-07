/**
 * @file cache.h
 * @brief Response caching interface
 *
 * This module provides a caching interface for protocol responses,
 * supporting different caching implementations through a common API.
 */

#ifndef CACHE_H
#define CACHE_H

#include "protocol.h"

/* === Function Declarations ====================================== */

/**
 * @brief Initialize the cache
 *
 * Sets up the cache with the specified capacity. Must be called
 * before any other cache operations.
 *
 * @param cap Maximum number of items the cache can hold
 * @return int OS_OK on success, OS_ERROR on failure
 */
int cache_init(size_t cap);

/**
 * @brief Retrieve a cached response
 *
 * Looks up a response in the cache by keyword.
 *
 * @param kw Keyword to look up
 * @param out Response structure to fill if found
 * @return int OS_OK if found, OS_ERROR if not found
 */
int cache_get(const char *kw, response_t *out);

/**
 * @brief Store a response in the cache
 *
 * Adds or updates a response in the cache for the given keyword.
 *
 * @param kw Keyword to store
 * @param resp Response to cache
 */
void cache_put(const char *kw, const response_t *resp);

/**
 * @brief Clean up cache resources
 *
 * Releases all resources used by the cache. Should be called
 * when the cache is no longer needed.
 */
void cache_cleanup(void);

#endif /* CACHE_H */
