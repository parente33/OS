/**
 * @file transport.h
 * @brief Transport layer for client-server communication using FIFOs
 *
 * This module implements a transport layer using named pipes (FIFOs) for
 * communication between clients and server. It supports both client and
 * server roles with proper connection handling and cleanup.
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <sys/types.h>

/** Maximum path length for FIFO names */
#define PATH_MAX 64

/** Transport layer roles */
typedef enum {
    TXP_CLIENT,  /* Client role */
    TXP_SERVER   /* Server role */
} txp_role_t;

/**
 * @brief Transport layer context structure
 *
 * Holds the state for a transport layer connection, including
 * file descriptors and paths for both client and server roles.
 */
typedef struct {
    txp_role_t   role;               /* Current role (client/server) */
    int          in_fd;              /* Input file descriptor */
    int          out_fd;             /* Output file descriptor */
    char         aux_path[PATH_MAX]; /* Auxiliary path (client FIFO) */
} txp_t;

/* === Life-cycle Functions ======================================== */

/**
 * @brief Open a client connection
 *
 * Creates a private FIFO for receiving responses and connects to the server.
 *
 * @param xp Pointer to transport context to initialize
 * @return int OS_OK on success, OS_ERROR on failure
 */
int txp_open_client(txp_t *xp);

/**
 * @brief Open a server connection
 *
 * Creates the main server FIFO and waits for client connections.
 *
 * @param xp Pointer to transport context to initialize
 * @return int OS_OK on success, OS_ERROR on failure
 */
int txp_open_server(txp_t *xp);

/* === I/O Functions ============================================== */

/**
 * @brief Read data from a file descriptor
 *
 * Reads exactly the requested number of bytes, handling partial reads.
 *
 * @param in_fd Input file descriptor
 * @param buf Buffer to store read data
 * @param cap Capacity of the buffer
 * @return int OS_OK on success, OS_ERROR on failure
 */
int txp_read(int in_fd, void *buf, size_t cap);

/**
 * @brief Write data to a file descriptor
 *
 * Writes exactly the requested number of bytes, handling partial writes
 * and retrying on interrupts.
 *
 * @param out_fd Output file descriptor
 * @param buf Buffer containing data to write
 * @param len Number of bytes to write
 * @return int OS_OK on success, OS_ERROR on failure
 */
int txp_write(int out_fd, const void *buf, size_t len);

/* === Server Helper Functions ==================================== */

/**
 * @brief Send a one-shot response to a client
 *
 * Opens the client's private FIFO, sends the response, and closes it.
 *
 * @param client_pid Process ID of the client
 * @param buf Buffer containing response data
 * @param len Length of response data
 * @return int OS_OK on success, OS_ERROR on failure
 */
int txp_reply(pid_t client_pid, const void *buf, size_t len);

/**
 * @brief Close a transport connection
 *
 * Closes all file descriptors and cleans up FIFOs based on role.
 *
 * @param xp Pointer to transport context to close
 */
void txp_close(txp_t *xp);

#endif /* TRANSPORT_H */
