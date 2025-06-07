/**
 * @file commands.c
 * @brief Implementation of command parsing and validation
 *
 * This module implements the command parsing and validation logic,
 * providing a robust interface for handling protocol commands.
 */

#include "commands.h"

#include <stdio.h>
#include <string.h>

#include "logger.h"

/* === Helper Macros ============================================== */

/**
 * @brief Create an array of argument types
 *
 * @param ... List of argument types
 * @return const arg_type_t[] Array of argument types
 */
#define ARG_ARR(...) (const arg_type_t[]){ __VA_ARGS__ }

/**
 * @brief Define a command row with arguments
 *
 * @param _flag Command flag
 * @param _min Minimum number of arguments
 * @param _op Operation code
 * @param _blocking Whether command blocks
 * @param ... List of argument types
 */
#define CMD_ROW(_flag, _min, _op, _blocking, ...)  \
{                                                  \
    .flag      = (_flag),                          \
    .types     = ARG_ARR(__VA_ARGS__),             \
    .argc_min  = (_min),                           \
    .argc_max  = sizeof(ARG_ARR(__VA_ARGS__))      \
        / sizeof(arg_type_t),                      \
    .opcode    = (_op),                            \
    .blocking  = (_blocking)                       \
}

/**
 * @brief Define a command row with no arguments
 *
 * @param _flag Command flag
 * @param _op Operation code
 * @param _blocking Whether command blocks
 */
#define CMD_ROW0(_flag, _op, _blocking)  \
{                                        \
    .flag     = (_flag),                 \
    .types    = NULL,                    \
    .argc_min = 0,                       \
    .argc_max = 0,                       \
    .opcode   = (_op),                   \
    .blocking = (_blocking)              \
}

/* === Command Table ============================================== */

/**
 * @brief Table of supported commands
 *
 * Each entry defines a command's flag, argument requirements,
 * operation code, and blocking behavior.
 */
const cmd_row_t cmd_table[] = {
    CMD_ROW("-a", 4, OP_A, 1, ARG_STR, ARG_STR, ARG_U32, ARG_STR),  /* Add document */
    CMD_ROW("-c", 1, OP_C, 0, ARG_U32),                             /* Check document */
    CMD_ROW("-d", 1, OP_D, 1, ARG_U32),                             /* Delete document */
    CMD_ROW("-l", 2, OP_L, 0, ARG_U32, ARG_STR),                    /* List documents */
    CMD_ROW("-s", 1, OP_S, 0, ARG_STR, ARG_U32),                    /* Search documents */
    CMD_ROW0("-f", OP_F, 1)                                         /* Flush */
};

/** Number of commands in the command table */
const size_t cmd_count = sizeof cmd_table / sizeof *cmd_table;

/* === Command Lookup Functions =================================== */

/**
 * @brief Look up a command by its operation code
 *
 * @param op Operation code to look up
 * @return const cmd_row_t* Command specification or NULL if not found
 */
const cmd_row_t*
cmd_by_opcode(opcode_t op)
{
    if (op >= OP_COUNT) {
        util_error("Invalid operation code: %d\n", op);
        return NULL;
    }
    return &cmd_table[op];  /* OP_A==0 maps to row 0 */
}

/**
 * @brief Look up a command by its flag
 *
 * @param flag Command flag to look up
 * @return const cmd_row_t* Command specification or NULL if not found
 */
static const cmd_row_t*
cmd_by_flag(const char *flag)
{
    if (!flag) {
        util_error("Invalid flag pointer\n");
        return NULL;
    }

    for (size_t i = 0; i < cmd_count; ++i) {
        if (strcmp(flag, cmd_table[i].flag) == 0)
            return &cmd_table[i];
    }

    util_error("Unknown command flag: %s\n", flag);
    return NULL;
}

/**
 * @brief Parse command-line arguments into a command specification
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return const cmd_row_t* Command specification or NULL if invalid
 */
const cmd_row_t*
command_parse(int argc, char *argv[])
{
    if (argc < 2) {
        util_error("No command specified\n");
        return NULL;
    }

    if (!argv) {
        util_error("Invalid argument array\n");
        return NULL;
    }

    const cmd_row_t *cmd = cmd_by_flag(argv[1]);
    if (!cmd) return NULL;

    int nargs = argc - 2;  /* Subtract program name and command flag */

    if (nargs < cmd->argc_min) {
        util_error("Too few arguments for %s (minimum %d)\n",
                  cmd->flag, cmd->argc_min);
        return NULL;
    }

    if (nargs > cmd->argc_max) {
        util_error("Too many arguments for %s (maximum %d)\n",
                  cmd->flag, cmd->argc_max);
        return NULL;
    }

    return cmd;
}
