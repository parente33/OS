/**
 * @file handlers_list.h
 * @brief List of protocol operation handlers
 *
 * This file defines the mapping between operation codes and their
 * handler functions. It is used by the X-macro pattern to generate
 * both declarations and the dispatch table.
 */

/* Format: X(OPCODE, handler_fn) */

/* Document Operations */
X(OP_A, handle_a)  /* Add document */
X(OP_C, handle_c)  /* Check document */
X(OP_D, handle_d)  /* Delete document */
X(OP_L, handle_l)  /* List documents */
X(OP_S, handle_s)  /* Search documents */

/* System Operations */
X(OP_F, handle_f)
