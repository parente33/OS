/**
 * @file dispatcher.c
 * @brief Implementation of request dispatching and argument processing
 *
 * This module implements the request dispatching logic, including
 * argument decoding, validation, and handler invocation.
 */

#include <signal.h>
#include <string.h>

#include "commands.h"
#include "protocol.h"
#include "transport.h"
#include "handlers.h"
#include "status.h"
#include "arg_codec.h"
#include "logger.h"

/* === Internal Functions ======================================== */

/**
 * @brief Process and decode request arguments
 *
 * Iterates through the request payload, decoding each TLV according
 * to the command specification. Validates argument types and ensures
 * all required arguments are present.
 *
 * @param req The request to process
 * @param row Command specification for argument validation
 * @param args Array to store decoded arguments
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
dismantle_request(const request_t *req, const cmd_row_t *row, arg_val_t args[])
{
    if (!req || !row || !args) {
        util_error("Invalid arguments to dismantle_request\n");
        return OS_ERROR;
    }

    proto_cursor_t cur;
    proto_cursor_init(&cur, req->payload,
                      req->hdr.len - sizeof req->hdr);

    for (uint8_t i = 0; i < row->argc_max; i++)
    {
        uint8_t t; const void *v; uint16_t l;

        int st = proto_cursor_next(&cur, &t, &v, &l);

        /* No more TLVs: ensure mandatory args were provided */
        if (st == OS_OK) {
            if (i < row->argc_min) {
                util_error("Missing required argument %d for %s\n",
                          i + 1, row->flag);
                return OS_ERROR;
            }
            /* Optional arguments absent – acceptable */
            break;
        }

        /* Check for corruption or wrong type */
        if (st != OS_AGAIN) {
            util_error("Protocol error in request payload\n");
            return OS_ERROR;
        }

        if (t != row->types[i]) {
            util_error("Invalid argument type %d for %s (expected %d)\n",
                      t, row->flag, row->types[i]);
            return OS_ERROR;
        }

        /* Decode using dispatch table */
        if (!arg_decoders[t]) {
            util_error("No decoder for argument type %d\n", t);
            return OS_ERROR;
        }

        if (arg_decoders[t](v, l, &args[i]) == OS_ERROR) {
            util_error("Failed to decode argument %d for %s\n",
                      i + 1, row->flag);
            return OS_ERROR;
        }
    }

    return OS_OK;
}

/* === Public Functions ========================================== */

/**
 * @brief Dispatch a protocol request to its handler
 *
 * @param req The request to process
 * @param row Command specification for argument validation
 * @param out Response structure to fill
 * @return int OS_OK on success, OS_ERROR on failure
 */
int
dispatch_request(const request_t *req, const cmd_row_t *row, response_t *out)
{
    if (!req || !row || !out) {
        util_error("Invalid arguments to dispatch_request\n");
        return OS_ERROR;
    }

    /* Allocate and initialize argument array */
    arg_val_t args[row->argc_max];
    memset(args, 0, sizeof args);

    /* Process and decode arguments */
    if (dismantle_request(req, row, args) == OS_ERROR) {
        return OS_ERROR;
    }

    /* Call handler to process request */
    return handlers[row->opcode](args, out);
}

