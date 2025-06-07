/**
 * @file dclient.c
 * @brief Client implementation for the distributed system
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "protocol.h"
#include "commands.h"
#include "transport.h"
#include "status.h"
#include "logger.h"
#include "arg_codec.h"

/**
 * @brief Processes and displays the response from the server
 *
 * @param rsp Pointer to the response structure
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
dismantle_response(const response_t *rsp)
{
    /* initialize the cursor over the payload */
    size_t payload_len = rsp->hdr.len - sizeof rsp->hdr;

    proto_cursor_t cur;
    proto_cursor_init(&cur, rsp->payload, payload_len);

    /* walk tlvs */
    uint8_t   type;
    uint16_t  vlen;
    const void *val;

    while (proto_cursor_next(&cur, &type, &val, &vlen) == OS_AGAIN)
    {
        arg_val_t decoded = {0};

        if (!arg_decoders[type] ||
            arg_decoders[type](val, vlen, &decoded) == OS_ERROR)
        {
            util_error("Corrupt TLV in response\n");
            return OS_ERROR;
        }

        switch (decoded.type)
        {
            case ARG_STR:
                util_log("%.*s\n", (int)decoded.v.str.len,
                                    decoded.v.str.ptr);
                break;

            case ARG_U32:
                util_log("%u\n", decoded.v.u32);
                break;

            default:
                util_log("[type %u len %u]\n", decoded.type, vlen);
                break;
        }
    }

    return OS_OK;
}

/**
 * @brief Builds a request packet from command line arguments
 *
 * @param req Pointer to request structure to fill
 * @param argv Array of command line arguments
 * @param argc Number of arguments
 * @param opcode Operation code for the request
 * @param types Array of argument types
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
build_request(request_t       *req,
              char *const      argv[],
              int              argc,
              uint8_t          opcode,
              const arg_type_t *types)
{
    proto_builder_t b;

    if (proto_req_init(req, &b, opcode)
        == OS_ERROR) return OS_ERROR;

    for (int i = 0; i < argc; i++)
    {
        arg_type_t type = types[i];
        if (!arg_encoders[type])
            return OS_ERROR;

        if (arg_encoders[type](&b, argv[i])
            == OS_ERROR) return OS_ERROR;
    }

    return proto_req_finish(req, &b);
}

/**
 * @brief Handles the connection to the server and request/response cycle
 *
 * @param req Pointer to the request to send
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
handle_conn(request_t *req)
{
    txp_t xp;
    if (txp_open_client(&xp) == OS_ERROR) {
        util_error("Failed to open client connection\n");
        return OS_ERROR;
    }

    if (proto_send_req(xp.out_fd, req) == OS_ERROR) {
        util_error("Failed to send request\n");
        goto fail;
    }

    response_t rsp;
    if (proto_recv_rsp(xp.in_fd, &rsp) == OS_ERROR) {
        util_error("Failed to receive response\n");
        goto fail;
    }

    int ret = dismantle_response(&rsp);

fail:
    txp_close(&xp);
    return ret;
}

/**
 * @brief Main entry point for the client
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return int EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int
main(int argc, char *argv[])
{
    const cmd_row_t *cmd = command_parse(argc, argv);
    if (!cmd) {
        util_error("Invalid command or arguments\n");
        return EXIT_FAILURE;
    }

    request_t req;
    if (build_request(&req, &argv[2], argc - 2,
                      cmd->opcode, cmd->types)
        == OS_ERROR) {
        util_error("Failed to build request\n");
        return EXIT_FAILURE;
    }

    int ret = handle_conn(&req);
    if (ret == OS_ERROR)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
