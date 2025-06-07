/**
 * @file arg_codec.c
 * @brief Implementation of argument encoding and decoding
 *
 * This module implements the encoding and decoding of protocol arguments
 * in TLV format. It provides type-safe conversion between wire format
 * and C data structures.
 */

#include "arg_codec.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "status.h"
#include "logger.h"

/* === Encoders =================================================== */

/**
 * @brief Encode a 32-bit unsigned integer argument
 *
 * Converts a string representation of a number to a uint32_t and
 * adds it to the protocol builder.
 *
 * @param b Protocol builder to append to
 * @param raw String containing the number
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
enc_u32(proto_builder_t *b, const char *raw)
{
    if (!b || !raw) {
        util_error("Invalid arguments to enc_u32\n");
        return OS_ERROR;
    }

    char *end = NULL;
    unsigned long val = strtoul(raw, &end, 10);

    /* Check for conversion errors */
    if (end == raw || *end != '\0') {
        util_error("Invalid number format: %s\n", raw);
        return OS_ERROR;
    }

    /* Check for overflow */
    if (val > UINT32_MAX) {
        util_error("Number too large: %s\n", raw);
        return OS_ERROR;
    }

    uint32_t v32 = (uint32_t)val;
    return proto_add_tlv(b, ARG_U32, &v32, sizeof v32);
}

/**
 * @brief Encode a string argument
 *
 * Adds a string to the protocol builder. The string is not null-terminated
 * in the wire format.
 *
 * @param b Protocol builder to append to
 * @param raw String to encode
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
enc_str(proto_builder_t *b, const char *raw)
{
    if (!b || !raw) {
        util_error("Invalid arguments to enc_str\n");
        return OS_ERROR;
    }

    size_t len = strlen(raw);
    if (len > UINT16_MAX) {
        util_error("String too long: %zu bytes\n", len);
        return OS_ERROR;
    }

    return proto_add_tlv(b, ARG_STR, raw, len);
}

/* === Decoders =================================================== */

/**
 * @brief Decode a 32-bit unsigned integer argument
 *
 * Extracts a uint32_t from the wire format and stores it in the
 * output structure.
 *
 * @param wire Raw wire data
 * @param len Length of wire data
 * @param out Output structure to fill
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
dec_u32(const void *wire, uint16_t len, arg_val_t *out)
{
    if (!wire || !out) {
        util_error("Invalid arguments to dec_u32\n");
        return OS_ERROR;
    }

    if (len != sizeof(uint32_t)) {
        util_error("Invalid length for u32: %u\n", len);
        return OS_ERROR;
    }

    out->type = ARG_U32;
    memcpy(&out->v.u32, wire, sizeof(uint32_t));

    return OS_OK;
}

/**
 * @brief Decode a string argument
 *
 * Extracts a string from the wire format and stores it in the
 * output structure. The string is not null-terminated.
 *
 * @param wire Raw wire data
 * @param len Length of wire data
 * @param out Output structure to fill
 * @return int OS_OK on success, OS_ERROR on failure
 */
static int
dec_str(const void *wire, uint16_t len, arg_val_t *out)
{
    if (!wire || !out) {
        util_error("Invalid arguments to dec_str\n");
        return OS_ERROR;
    }

    out->type = ARG_STR;
    out->v.str.ptr = (const char *)wire;
    out->v.str.len = len;

    return OS_OK;
}

/* === Dispatch Tables ============================================ */

const arg_encode_fn arg_encoders[ARG_COUNT] = {
    [ARG_U32] = enc_u32,
    [ARG_STR] = enc_str,
};

const arg_decode_fn arg_decoders[ARG_COUNT] = {
    [ARG_U32] = dec_u32,
    [ARG_STR] = dec_str,
};