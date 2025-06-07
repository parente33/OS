/**
 * @file cache.c
 * @brief Cache implementation selection
 *
 * This module selects the appropriate cache implementation based on
 * the CACHE_IMPL definition. It supports multiple caching strategies
 * through a common interface.
 */

#include "cache.h"

/* === Cache Implementation Types ================================= */

/** No caching implementation */
#define CACHE_NONE   0

/** Least Recently Used (LRU) cache implementation */
#define CACHE_LRU    1

/* === Implementation Selection ================================== */

#ifndef CACHE_IMPL
#  define CACHE_IMPL CACHE_LRU  /* Default to LRU implementation */
#endif

/* Include the selected implementation */
#if (CACHE_IMPL) == CACHE_NONE
#   include "cache_none.c"
#elif (CACHE_IMPL) == CACHE_LRU
#   include "cache_lru.c"
#else
#   error "Unknown cache implementation: CACHE_IMPL must be CACHE_NONE or CACHE_LRU"
#endif
