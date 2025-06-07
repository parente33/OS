/**
 * @file cache_lru.c
 * @brief Least Recently Used (LRU) cache implementation
 *
 * This module implements an LRU cache for protocol responses using
 * a hash table for O(1) lookups and a doubly-linked list for LRU
 * ordering. It also provides persistence through disk storage.
 */

#include <glib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "transport.h"
#include "cache.h"
#include "logger.h"
#include "status.h"

/* === Constants ================================================== */

/** Path to the cache persistence file */
#define CACHE_FILE "tmp/cache_lru.bin"

/** Maximum length of a cache key */
#define MAX_KEY_LEN 255

/* === Type Definitions =========================================== */

/**
 * @brief Cache entry structure
 *
 * Represents a single entry in the LRU cache, containing the key,
 * response data, and links for the LRU ordering.
 */
typedef struct entry {
    char            *key;      /* Cache key (owned) */
    response_t       rsp;      /* Cached response */
    struct entry    *prev;     /* Previous entry in LRU order */
    struct entry    *next;     /* Next entry in LRU order */
} entry_t;

/* === Internal State ============================================ */

/** Hash table mapping keys to entries */
static GHashTable *ht    = NULL;

/** Most recently used entry */
static entry_t    *head  = NULL;

/** Least recently used entry */
static entry_t    *tail  = NULL;

/** Maximum number of cache entries */
static size_t      cap   = 0;

/** Current number of cache entries */
static size_t      count = 0;

/* === Forward Declarations ====================================== */

static void load_from_disk(void);
static void dump_to_disk(void);

/* === Internal Functions ======================================== */

/**
 * @brief Move an entry to the front of the LRU list
 *
 * @param e Entry to move
 */
static void
move_front(entry_t *e)
{
    if (!e || e == head) return;

    /* Remove from current position */
    if (e->prev) e->prev->next = e->next;
    if (e->next) e->next->prev = e->prev;
    if (e == tail) tail = e->prev;

    /* Move to front */
    e->prev = NULL;
    e->next = head;
    if (head) head->prev = e;
    head = e;
    if (!tail) tail = e;
}

/**
 * @brief Evict entries to maintain capacity
 *
 * Removes least recently used entries until the cache is within
 * its capacity limit.
 */
static void
evict(void)
{
    while (cap && count > cap && tail)
    {
        entry_t *old = tail;
        tail = old->prev;
        if (tail) tail->next = NULL;
        else      head = NULL;

        g_hash_table_remove(ht, old->key);  /* frees key */
        free(old);
        --count;
    }
}

/**
 * @brief Load cache entries from disk
 *
 * Reads cached entries from the persistence file and adds them
 * to the cache up to the capacity limit.
 */
static void
load_from_disk(void)
{
    int fd = open(CACHE_FILE, O_RDONLY);
    if (fd < 0) {
        if (errno != ENOENT) {
            util_error("Failed to open cache file: %s\n", strerror(errno));
        }
        return;
    }

    uint32_t n;
    if (txp_read(fd, &n, sizeof n) < 0) {
        util_error("Failed to read cache entry count\n");
        close(fd);
        return;
    }

    for (uint32_t i = 0; i < n && count < cap; ++i)
    {
        uint16_t klen, rlen;
        if (txp_read(fd, &klen, sizeof klen) < 0) {
            util_error("Failed to read key length\n");
            break;
        }

        if (klen == 0 || klen > MAX_KEY_LEN) {
            util_error("Invalid key length: %u\n", klen);
            break;
        }

        char key[MAX_KEY_LEN + 1];
        if (txp_read(fd, key, klen) < 0) {
            util_error("Failed to read key\n");
            break;
        }
        key[klen] = '\0';

        if (txp_read(fd, &rlen, sizeof rlen) < 0) {
            util_error("Failed to read response length\n");
            break;
        }

        if ((size_t)rlen > sizeof(response_t)) {
            util_error("Response too large: %u bytes\n", rlen);
            break;
        }

        response_t rsp;
        if (txp_read(fd, &rsp, rlen) < 0) {
            util_error("Failed to read response\n");
            break;
        }

        entry_t *e = malloc(sizeof *e);
        if (!e) {
            util_error("Failed to allocate cache entry\n");
            break;
        }

        e->key = g_strdup(key);
        e->rsp = rsp;
        e->prev = NULL;
        e->next = head;
        if (head) head->prev = e;
        head = e;
        if (!tail) tail = e;

        g_hash_table_insert(ht, e->key, e);
        ++count;
    }

    close(fd);
}

/**
 * @brief Save cache entries to disk
 *
 * Writes all cached entries to the persistence file.
 */
static void
dump_to_disk(void)
{
    int fd = open(CACHE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0660);
    if (fd < 0) {
        util_error("Failed to create cache file: %s\n", strerror(errno));
        return;
    }

    uint32_t n = (uint32_t)count;
    if (txp_write(fd, &n, sizeof n) < 0) {
        util_error("Failed to write cache entry count\n");
        close(fd);
        return;
    }

    for (entry_t *e = head; e; e = e->next)
    {
        uint16_t klen = (uint16_t)strlen(e->key);
        uint16_t rlen = e->rsp.hdr.len;

        if (txp_write(fd, &klen, sizeof klen) < 0 ||
            txp_write(fd, e->key, klen) < 0       ||
            txp_write(fd, &rlen, sizeof rlen) < 0 ||
            txp_write(fd, &e->rsp, rlen) < 0) {
            util_error("Failed to write cache entry\n");
            break;
        }
    }

    close(fd);
}

/* === Public Functions ========================================== */

/**
 * @brief Initialize the LRU cache
 *
 * @param max_entries Maximum number of cache entries
 * @return int OS_OK on success, OS_ERROR on failure
 */
int
cache_init(size_t max_entries)
{
    if (ht) {
        util_error("Cache already initialized\n");
        return OS_ERROR;
    }

    ht = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    if (!ht) {
        util_error("Failed to create hash table\n");
        return OS_ERROR;
    }

    cap = max_entries;
    head = tail = NULL;
    count = 0;

    if (cap) {
        load_from_disk();
    }

    return OS_OK;
}

/**
 * @brief Retrieve a response from the cache
 *
 * @param kw Keyword to look up
 * @param out Response structure to fill if found
 * @return int OS_OK if found, OS_ERROR if not found
 */
int
cache_get(const char *kw, response_t *out)
{
    if (!kw || !out) {
        util_error("Invalid arguments to cache_get\n");
        return OS_ERROR;
    }

    entry_t *e = g_hash_table_lookup(ht, kw);
    if (!e) return OS_ERROR;

    move_front(e);
    *out = e->rsp;
    return OS_OK;
}

/**
 * @brief Store a response in the cache
 *
 * @param kw Keyword to store
 * @param rsp Response to cache
 */
void
cache_put(const char *kw, const response_t *rsp)
{
    if (!kw || !rsp || !cap) return;

    entry_t *e = g_hash_table_lookup(ht, kw);
    if (e) {
        e->rsp = *rsp;
        move_front(e);
        return;
    }

    e = malloc(sizeof *e);
    if (!e) {
        util_error("Failed to allocate cache entry\n");
        return;
    }

    e->key = g_strdup(kw);
    e->rsp = *rsp;
    e->prev = NULL;
    e->next = head;
    if (head) head->prev = e;
    head = e;
    if (!tail) tail = e;

    g_hash_table_insert(ht, e->key, e);
    ++count;
    evict();
}

/**
 * @brief Clean up cache resources
 *
 * Saves the cache to disk and releases all resources.
 */
void
cache_cleanup(void)
{
    if (cap) {
        dump_to_disk();
    }

    for (entry_t *e = head; e; ) {
        entry_t *nxt = e->next;
        free(e);
        e = nxt;
    }

    if (ht) {
        g_hash_table_destroy(ht);
    }

    ht = NULL;
    head = tail = NULL;
    count = cap = 0;
}
