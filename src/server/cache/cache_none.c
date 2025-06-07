/**
 * @file cache_none.c
 * @brief No-cache implementation
 *
 * This module provides a null implementation of the cache interface
 * that performs no caching. All operations are no-ops that return
 * appropriate error codes.
 */

#include "cache.h"

/* === Public Functions ========================================== */

/**
 * @brief Initialize the no-cache implementation
 *
 * @param n Ignored capacity parameter
 * @return int Always returns OS_OK
 */
int
cache_init(size_t n)
{
    (void)n;  /* Unused parameter */
    return OS_OK;
}

/**
 * @brief Attempt to retrieve from no-cache
 *
 * @param k Ignored keyword parameter
 * @param r Ignored response parameter
 * @return int Always returns OS_ERROR (not found)
 */
int
cache_get(const char *k, response_t *r)
{
    (void)k;  /* Unused parameter */
    (void)r;  /* Unused parameter */
    return OS_ERROR;
}

/**
 * @brief Attempt to store in no-cache
 *
 * @param k Ignored keyword parameter
 * @param r Ignored response parameter
 */
void
cache_put(const char *k, const response_t *r)
{
    (void)k;  /* Unused parameter */
    (void)r;  /* Unused parameter */
}

/**
 * @brief Clean up no-cache resources
 *
 * No resources to clean up in this implementation.
 */
void
cache_cleanup(void)
{
    /* Nothing to clean up */
}
