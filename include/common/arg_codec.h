/**
 * @file arg_codec.h
 * @brief Argument encoding and decoding for protocol messages
 *
 * This module provides functions for encoding and decoding arguments
 * in the protocol's TLV format. It supports various data types and
 * provides a clean interface for argument handling.
 */

#ifndef ARG_CODEC_H
#define ARG_CODEC_H

#include <stdint.h>
#include "protocol.h"

/* === Type Definitions ============================================ */

/**
 * @brief Decoded representation of a wire TLV value
 *
 * This structure provides a type-safe way to handle decoded TLV values.
 * Handlers receive an array of these – no byte-swapping or length checks
 * required on their side.
 */
typedef struct {
    arg_type_t type;  /* Type of the argument */
    union {
        struct {                  /* String argument (ARG_STR) */
            const char *ptr;      /* Pointer to string data (not null-terminated) */
            uint16_t    len;      /* Original byte length */
        } str;
        uint32_t  u32;           /* 32-bit unsigned integer (ARG_U32) */
    } v;
} arg_val_t;

/* === Function Types ============================================== */

/**
 * @brief Function type for encoding arguments
 *
 * @param b Protocol builder to append to
 * @param raw Raw string value to encode
 * @return int OS_OK on success, OS_ERROR on failure
 */
typedef int (*arg_encode_fn)(proto_builder_t *b, const char *raw);

/**
 * @brief Function type for decoding arguments
 *
 * @param wire Raw wire data to decode
 * @param len Length of wire data
 * @param out Output structure to fill
 * @return int OS_OK on success, OS_ERROR on failure
 */
typedef int (*arg_decode_fn)(const void *wire, uint16_t len, arg_val_t *out);

/* === Dispatch Tables ============================================ */

/**
 * @brief Table of argument encoders
 *
 * Maps argument types to their encoding functions.
 * An entry may be NULL if the type is not supported.
 */
extern const arg_encode_fn arg_encoders[ARG_COUNT];

/**
 * @brief Table of argument decoders
 *
 * Maps argument types to their decoding functions.
 * An entry may be NULL if the type is not supported.
 */
extern const arg_decode_fn arg_decoders[ARG_COUNT];

#endif /* ARG_CODEC_H */