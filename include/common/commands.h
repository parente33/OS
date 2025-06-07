/**
 * @file commands.h
 * @brief Command parsing and validation for the protocol
 *
 * This module defines the command structure and provides functions
 * for parsing and validating command-line arguments against the
 * protocol specification.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>
#include <stdint.h>

/* === Constants ================================================== */

/** Maximum number of arguments a command can accept */
#define MAX_ARGS 4

/* === Type Definitions =========================================== */

/**
 * @brief Supported argument types in the protocol
 */
typedef enum {
    ARG_U32 = 0,          /* 32-bit unsigned integer in network byte order */
    ARG_STR = 1,          /* NULL-terminated UTF-8 string */
    ARG_COUNT
} arg_type_t;

/**
 * @brief Protocol operation codes
 */
typedef enum {
    OP_A = 0,            /* Add document operation */
    OP_C = 1,            /* Check document operation */
    OP_D = 2,            /* Delete document operation */
    OP_L = 3,            /* List documents operation */
    OP_S = 4,            /* Search documents operation */
    OP_F = 5,            /* Flush operation */
    OP_COUNT
} opcode_t;

/**
 * @brief Command specification structure
 *
 * Defines the format and requirements for a protocol command,
 * including its flag, argument types, and operational characteristics.
 */
typedef struct {
    const char       *flag;      /* Command-line flag (e.g. "-a") */
    const arg_type_t *types;     /* Array of expected argument types */
    uint8_t           argc_min;  /* Minimum number of required arguments */
    uint8_t           argc_max;  /* Maximum number of allowed arguments */
    opcode_t          opcode;    /* Protocol operation code */
    unsigned int      blocking;  /* Whether command blocks until complete */
} cmd_row_t;

/* === Global Variables =========================================== */

/** Command table defining all supported commands */
extern const cmd_row_t  cmd_table[];

/** Number of commands in the command table */
extern const size_t     cmd_count;

/* === Function Declarations ====================================== */

/**
 * @brief Look up a command by its operation code
 *
 * @param op Operation code to look up
 * @return const cmd_row_t* Command specification or NULL if not found
 */
const cmd_row_t* cmd_by_opcode(opcode_t op);

/**
 * @brief Parse command-line arguments into a command specification
 *
 * Validates the command-line arguments against the command table
 * and returns the matching command specification.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return const cmd_row_t* Command specification or NULL if invalid
 */
const cmd_row_t* command_parse(int argc, char *argv[]);

#endif /* COMMANDS_H */
