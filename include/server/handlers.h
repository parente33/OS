/**
 * @file handlers.h
 * @brief Protocol operation handlers
 *
 * This module defines the handler functions for each protocol operation
 * and provides a dispatch mechanism for routing operations to their
 * respective handlers.
 */

#ifndef HANDLERS_H
#define HANDLERS_H

#include "protocol.h"
#include "arg_codec.h"

/* === Type Definitions =========================================== */

/**
 * @brief Function type for protocol operation handlers
 *
 * Each handler processes a specific protocol operation by taking
 * decoded arguments and producing a response.
 *
 * @param argv Array of decoded argument values
 * @param out Response structure to fill
 * @return int OS_OK on success, OS_ERROR on failure
 */
typedef int (*handler_fn)(const arg_val_t argv[], response_t *out);

/* === Handler Declarations ======================================= */

/* Build extern declarations automatically */
#define X(op, fn) extern int fn(const arg_val_t argv[], response_t *out);
    #include "handlers_list.h"
#undef X

/* === Dispatch Table ============================================ */

/**
 * @brief Table of operation handlers
 *
 * Maps operation codes to their handler functions.
 * The table is indexed by opcode_t values.
 */
extern const handler_fn handlers[OP_COUNT];

#endif /* HANDLERS_H */