/**
 * @file handlers.c
 * @brief Implementation of protocol operation handlers
 *
 * This module implements the dispatch table for protocol operations,
 * mapping each operation code to its corresponding handler function.
 */

#include "handlers.h"

/* === Dispatch Table Implementation ============================== */

/**
 * @brief Table of operation handlers
 *
 * This table is automatically generated from handlers_list.h using
 * the X-macro pattern. Each entry maps an operation code to its
 * handler function.
 */
#define X(op, fn) [op] = fn,
const handler_fn handlers[OP_COUNT] = {
    #include "handlers_list.h"
};
#undef X
