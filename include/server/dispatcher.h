/**
 * @file dispatcher.h
 * @brief Request dispatching and argument processing
 *
 * This module handles the dispatching of protocol requests to their
 * appropriate handlers, including argument decoding and validation.
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "protocol.h"
#include "commands.h"

/* === Function Declarations ====================================== */

/**
 * @brief Dispatch a protocol request to its handler
 *
 * Processes the request payload, decodes arguments according to the
 * command specification, and calls the appropriate handler function.
 *
 * @param req The request to process
 * @param row Command specification for argument validation
 * @param out Response structure to fill
 * @return int OS_OK on success, OS_ERROR on failure
 */
int dispatch_request(const request_t *req, const cmd_row_t *row, response_t *out);

#endif /* DISPATCHER_H */
