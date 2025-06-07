/**
 * @file transport.c
 * @brief Implementation of the transport layer using FIFOs
 */

#include "transport.h"

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "status.h"
#include "protocol.h"
#include "logger.h"

/* Path to the server's request FIFO */
#define TXP_REQ_FIFO       "/tmp/server.fifo"
/* Format string for client response FIFO paths */
#define TXP_RSP_FMT        "/tmp/client_%d.fifo"
/* Permissions for FIFO creation */
#define TXP_PERM           0600
/* Maximum message size */
#define TXP_MAX            RSP_MAX

/* === Internal Helpers ============================================ */

/**
 * @brief Generate path for client FIFO
 *
 * @param dst Destination buffer for path
 * @param n Size of destination buffer
 * @param pid Client process ID
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
path_for_pid(char *dst, size_t n, pid_t pid)
{
    if (!dst || n == 0) {
        util_error("Invalid destination buffer\n");
        return OS_ERROR;
    }

    int ret = snprintf(dst, n, TXP_RSP_FMT, pid);
    if (ret < 0 || (size_t)ret >= n) {
        util_error("Path buffer too small for PID %d\n", pid);
        return OS_ERROR;
    }
    return OS_OK;
}

/**
 * @brief Create a FIFO if it doesn't exist
 *
 * @param p Path to FIFO
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
mkfifo_once(const char *p)
{
    if (!p) {
        util_error("Invalid FIFO path\n");
        return OS_ERROR;
    }

    if (mkfifo(p, TXP_PERM) == -1) {
        if (errno != EEXIST) {
            util_error("Failed to create FIFO %s: %s\n", p, strerror(errno));
            return OS_ERROR;
        }
    }
    return OS_OK;
}

/**
 * @brief Check if server is available
 *
 * @return int OS_OK if server is listening, OS_ERROR otherwise
 */
static int
txp_check_server(void)
{
    int fd = open(TXP_REQ_FIFO, O_WRONLY | O_NONBLOCK);
    if (fd >= 0) {
        close(fd);
        return OS_OK;                /* Server is listening */
    }

    util_error("Server not available: %s\n", strerror(errno));
    return OS_ERROR;
}

/* === Client Functions ============================================ */

int
txp_open_client(txp_t *xp)
{
    if (!xp) {
        util_error("Invalid transport context\n");
        return OS_ERROR;
    }

    memset(xp, 0, sizeof *xp);
    xp->role = TXP_CLIENT;

    /* Create private FIFO for responses */
    if (path_for_pid(xp->aux_path, sizeof xp->aux_path, getpid())
        == OS_ERROR) return OS_ERROR;

    (void) unlink(xp->aux_path);  /* Remove if exists */

    if (mkfifo_once(xp->aux_path) == OS_ERROR) {
        util_error("Failed to create client FIFO\n");
        return OS_ERROR;
    }

    /* Check if server is available */
    if (txp_check_server() == OS_ERROR) {
        unlink(xp->aux_path);  /* Clean up on failure */
        return OS_ERROR;
    }

    /* Open our FIFO for reading responses */
    xp->in_fd = open(xp->aux_path, O_RDWR);
    if (xp->in_fd == -1) {
        util_error("Failed to open client FIFO: %s\n", strerror(errno));
        unlink(xp->aux_path);
        return OS_ERROR;
    }

    /* Open server FIFO for sending requests */
    xp->out_fd = open(TXP_REQ_FIFO, O_WRONLY | O_NONBLOCK);
    if (xp->out_fd == -1) {
        util_error("Failed to open server FIFO: %s\n", strerror(errno));
        close(xp->in_fd);
        unlink(xp->aux_path);
        return OS_ERROR;
    }

    return OS_OK;
}

/* === Server Functions ============================================ */

int
txp_open_server(txp_t *xp)
{
    if (!xp) {
        util_error("Invalid transport context\n");
        return OS_ERROR;
    }

    memset(xp, 0, sizeof *xp);
    xp->role = TXP_SERVER;

    /* Create server FIFO path */
    if (snprintf(xp->aux_path, PATH_MAX, "%s", TXP_REQ_FIFO) >= PATH_MAX) {
        util_error("Server FIFO path too long\n");
        return OS_ERROR;
    }

    (void) unlink(TXP_REQ_FIFO);  /* Remove if exists */

    if (mkfifo_once(TXP_REQ_FIFO) == OS_ERROR) {
        util_error("Failed to create server FIFO\n");
        return OS_ERROR;
    }

    /* Open server FIFO for reading requests */
    xp->in_fd = open(TXP_REQ_FIFO, O_RDWR);
    if (xp->in_fd == -1) {
        util_error("Failed to open server FIFO: %s\n", strerror(errno));
        unlink(TXP_REQ_FIFO);
        return OS_ERROR;
    }

    /* Server has no persistent write FD */
    xp->out_fd = -1;

    return OS_OK;
}

/* === I/O Functions ============================================== */

int
txp_read(int in_fd, void *buf, size_t cap)
{
    if (!buf || in_fd < 0) {
        util_error("Invalid read parameters\n");
        return OS_ERROR;
    }

    size_t got = 0;
    uint8_t *p = buf;

    while (got < cap) {
        ssize_t r = read(in_fd, p + got, cap - got);
        if (r <= 0) {
            if (r == 0) {
                util_error("EOF while reading\n");
            } else if (errno != EINTR) {
                util_error("Read error: %s\n", strerror(errno));
            }
            return OS_ERROR;
        }
        got += (size_t)r;
    }

    return OS_OK;
}

int
txp_write(int out_fd, const void *buf, size_t len)
{
    if (!buf || out_fd < 0) {
        util_error("Invalid write parameters\n");
        return OS_ERROR;
    }

    const uint8_t *p = buf;
    size_t sent = 0;

    while (sent < len) {
        ssize_t w = write(out_fd, p + sent, len - sent);
        if (w < 0) {
            if (errno == EINTR) {
                continue;               /* Interrupted → retry */
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Pipe/FIFO full – give the reader a chance */
                continue;
            }

            util_error("Write error: %s\n", strerror(errno));
            return OS_ERROR;
        }
        sent += (size_t)w;
    }
    return OS_OK;
}

/* === Server Helper Functions ==================================== */

int
txp_reply(pid_t pid, const void *buf, size_t len)
{
    if (!buf || len == 0 || len > TXP_MAX || pid <= 0) {
        util_error("Invalid reply parameters\n");
        return OS_ERROR;
    }

    char path[64];
    if (path_for_pid(path, sizeof path, pid) != OS_OK) {
        return OS_ERROR;
    }

    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        util_error("Failed to open client FIFO for reply: %s\n", strerror(errno));
        return OS_ERROR;
    }

    int rc = txp_write(fd, buf, len);
    close(fd);                       /* Avoid descriptor leak */
    return rc;
}

/* === Cleanup Functions ========================================== */

void
txp_close(txp_t *xp)
{
    if (!xp) return;

    /* Always close any open FDs */
    if (xp->in_fd >= 0) {
        close(xp->in_fd);
        xp->in_fd = -1;
    }
    if (xp->out_fd >= 0) {
        close(xp->out_fd);
        xp->out_fd = -1;
    }

    /* If we were a client, remove only our private FIFO */
    if (xp->role == TXP_CLIENT && xp->aux_path[0]) {
        unlink(xp->aux_path);
        xp->aux_path[0] = '\0';
    }

    /* If we were the server, remove the well-known FIFO */
    if (xp->role == TXP_SERVER) {
        unlink(TXP_REQ_FIFO);
    }

    /* Reset role so we don't try again by accident */
    xp->role = 0;
}
