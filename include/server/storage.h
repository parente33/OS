/**
 * @file storage.h
 * @brief Persistent storage layer for document management
 *
 * This module implements a simple file-based storage system for documents.
 * It provides basic CRUD operations with a fixed-size record format.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#include "document.h"

/** Path to the storage file */
#define STG_FILE_PATH "tmp/index.bin"

/* === Life-cycle Functions ======================================== */

/**
 * @brief Initialize the storage system
 *
 * Opens or creates the storage file. Must be called before any other
 * storage operations.
 *
 * @return int OS_OK on success, OS_ERROR on failure
 */
int stg_init   (void);

/**
 * @brief Close the storage system
 *
 * Closes the storage file and releases resources.
 *
 * @return int OS_OK on success, OS_ERROR on failure
 */
int stg_close  (void);

/* === Document Operations ========================================= */

/**
 * @brief Add a new document to storage
 *
 * Appends the document to the end of the storage file and assigns
 * it a new key.
 *
 * @param doc Pointer to document to store
 * @return int The assigned key on success, OS_ERROR on failure
 */
int stg_add_doc  (document_t *doc);

/**
 * @brief Retrieve a document from storage
 *
 * Reads a document by its key from the storage file.
 *
 * @param key Document key to retrieve
 * @param out Pointer to store retrieved document
 * @return int OS_OK on success, OS_ERROR on failure
 */
int stg_get_doc  (int key, document_t *out);

/**
 * @brief Delete a document from storage
 *
 * Marks a document as deleted by setting its key to -1.
 * The space is not reclaimed.
 *
 * @param key Document key to delete
 * @return int OS_OK on success, OS_ERROR on failure
 */
int stg_del_doc  (int key);

/**
 * @brief Get total number of document slots
 *
 * Returns the total number of document slots in the storage file,
 * including deleted documents.
 *
 * @return int Number of slots on success, OS_ERROR on failure
 */
int stg_total    (void);

#endif /* STORAGE_H */