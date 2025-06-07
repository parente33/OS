/**
 * @file storage.c
 * @brief Implementation of the persistent storage layer
 */

#include "storage.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "document.h"
#include "status.h"
#include "logger.h"

/* === Internal State ============================================= */

/** File descriptor for the storage file */
static int stg_fd = -1;

/* === Internal Helpers =========================================== */

/**
 * @brief Calculate file offset for a document key
 *
 * @param key Document key
 * @return off_t File offset
 */
static off_t
get_offset(int key)
{
    return (off_t)key * sizeof(document_t);
}

/**
 * @brief Open or create the storage file
 *
 * Opens the storage file in read/write mode, creating it if it doesn't exist.
 *
 * @return int File descriptor on success, -1 on failure
 */
static int
stg_open_file(void)
{
    /* Read/write, create if missing, 0600 permissions */
    stg_fd = open(STG_FILE_PATH, O_RDWR | O_CREAT, 0600);

    if (stg_fd == -1) {
        util_error("Could not create or open storage file: %s\n",
                  strerror(errno));
        return -1;
    }

    return stg_fd;
}

/* === Life-cycle Functions ======================================== */

int
stg_init(void)
{
    if (stg_fd != -1) {
        util_error("Storage already initialized\n");
        return OS_ERROR;
    }

    return stg_open_file() == -1 ? OS_ERROR : OS_OK;
}

int
stg_close(void)
{
    if (stg_fd != -1) {
        if (close(stg_fd) < 0) {
            util_error("Could not close storage file: %s\n", strerror(errno));
            return OS_ERROR;
        }
        stg_fd = -1;
    }
    return OS_OK;
}

/* === Document Operations ========================================= */

int
stg_add_doc(document_t *doc)
{
    if (!doc) {
        util_error("Invalid document pointer\n");
        return OS_ERROR;
    }

    if (stg_fd == -1 && stg_open_file() == -1) {
        return OS_ERROR;
    }

    /* Find current end to get key */
    off_t end_off = lseek(stg_fd, 0, SEEK_END);
    if (end_off < 0) {
        util_error("Failed to seek to end of file: %s\n", strerror(errno));
        return OS_ERROR;
    }

    int key = (int)(end_off / (off_t)sizeof(document_t));

    document_t tmp = *doc;
    tmp.key = key;

    /* Write at end */
    if (write(stg_fd, &tmp, sizeof tmp) != sizeof tmp) {
        util_error("Failed to write document: %s\n", strerror(errno));
        return OS_ERROR;
    }

    return key;
}

int
stg_get_doc(int key, document_t *out)
{
    if (!out) {
        util_error("Invalid output pointer\n");
        return OS_ERROR;
    }

    if (key < 0) {
        util_error("Invalid document key: %d\n", key);
        return OS_ERROR;
    }

    if (stg_fd == -1 && stg_open_file() == -1) {
        return OS_ERROR;
    }

    /* Verify key within file size */
    off_t end_off = lseek(stg_fd, 0, SEEK_END);
    if (end_off < 0) {
        util_error("Failed to seek to end of file: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (get_offset(key) + (off_t)sizeof(document_t) > end_off) {
        util_error("Document key out of range: %d\n", key);
        return OS_ERROR;
    }

    /* Seek and read */
    off_t off = get_offset(key);
    if (lseek(stg_fd, off, SEEK_SET) < 0) {
        util_error("Failed to seek to document: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (read(stg_fd, out, sizeof *out) != sizeof *out) {
        util_error("Failed to read document: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (out->key != key) {
        util_error("Document corrupted or deleted: %d\n", key);
        return OS_ERROR;
    }

    return OS_OK;
}

int
stg_del_doc(int key)
{
    if (key < 0) {
        util_error("Invalid document key: %d\n", key);
        return OS_ERROR;
    }

    if (stg_fd == -1 && stg_open_file() == -1) {
        return OS_ERROR;
    }

    /* Verify key within file size */
    off_t end_off = lseek(stg_fd, 0, SEEK_END);
    if (end_off < 0) {
        util_error("Failed to seek to end of file: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (get_offset(key) + (off_t)sizeof(document_t) > end_off) {
        util_error("Document key out of range: %d\n", key);
        return OS_ERROR;
    }

    /* Read existing record to verify it is still active */
    off_t off = get_offset(key);
    if (lseek(stg_fd, off, SEEK_SET) < 0) {
        util_error("Failed to seek to document: %s\n", strerror(errno));
        return OS_ERROR;
    }

    document_t cur;
    if (read(stg_fd, &cur, sizeof cur) != sizeof cur) {
        util_error("Failed to read document: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (cur.key != key) {
        util_error("Document already deleted or corrupted: %d\n", key);
        return OS_ERROR;
    }

    /* Mark as tombstone */
    document_t tomb;
    memset(&tomb, 0, sizeof tomb);
    tomb.key = -1;

    /* Seek back and overwrite */
    if (lseek(stg_fd, off, SEEK_SET) < 0) {
        util_error("Failed to seek to document: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (write(stg_fd, &tomb, sizeof tomb) != sizeof tomb) {
        util_error("Failed to write tombstone: %s\n", strerror(errno));
        return OS_ERROR;
    }

    return OS_OK;
}

int
stg_total(void)
{
    if (stg_fd == -1 && stg_open_file() == -1) {
        return OS_ERROR;
    }

    off_t end_off = lseek(stg_fd, 0, SEEK_END);
    if (end_off < 0) {
        util_error("Failed to seek to end of file: %s\n", strerror(errno));
        return OS_ERROR;
    }

    return (int)(end_off / (off_t)sizeof(document_t));
}
