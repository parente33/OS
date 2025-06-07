/**
 * @file protocol.h
 * @brief Implementation of the binary TLV protocol
 */


#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
    ┌────────────────────────────────────────────────┐
    │  Format: Little-endian                         │
    │  Encoding: TLV (Type-Length-Value)             │
    │  Alignment: Packed (no padding between fields) │
    │  Max size: 512 bytes (header + all TLVs)       │
    └────────────────────────────────────────────────┘

    ┌────────────────────────────────────────────────────────────────────────┐
    │                        REQUEST LAYOUT IN MEMORY                        │
    ├────────────────────┬───────────────────────────────────────────────────┤
    │  header (7 bytes)  │     payload (≈500 bytes of back-to-back TLVs)     │
    └────────────────────┴───────────────────────────────────────────────────┘

    ─────────────────────────────────────────────────────────────────────────

    EXAMPLE: program -s "banana" "42"

    — After header begins TLV (type-length-value) entries —
    ┌────────────────────────────────────────────────────────────────┐
    │  TLV #0                                                        │
    ├───────────┬─────────┬──────────────────────────────────────────┤
    │  type (1) │ len (2) │ value (6 bytes: 'b','a','n','a','n','a') │
    └───────────┴─────────┴──────────────────────────────────────────┘

    ┌────────────────────────────────────────────────────────────────┐
    │  TLV #1                                                        │
    ├───────────┬─────────┬──────────────────────────────────────────┤
    │  type (1) │ len (2) │ value (4 bytes: 0x2A 0x00 0x00 0x00)     │
    └───────────┴─────────┴──────────────────────────────────────────┘

    — Memory snapshot (hex & ASCII) —
    ┌────────┬──────────────────────┬─────────────────────────┐
    │ Offset │ Hex dump             │ ASCII                   │
    ├────────┼──────────────────────┼─────────────────────────┤
    │  0x00  │ 19 00                │ hdr.len    = 23         │
    │  0x02  │ 05                   │ hdr.opcode = 5          │
    │  0x03  │ D2 04 00 00          │ hdr.pid    = 1234       │
    ├────────┼──────────────────────┼─────────────────────────┤
    │  0x07  │ 01                   │ t1.type = 2             │
    │  0x08  │ 07 00                │ t1.len  = 6             │
    │  0x0A  │ 62 61 6E 61 6E 61    │ value   = "banana"      │
    ├────────┼──────────────────────┼─────────────────────────┤
    │  0x10  │ 02                   │ t2.type = 1             │
    │  0x11  │ 04 00                │ t2.len  = 4             │
    │  0x13  │ 2A 00 00 00          │ value   = 42            │
    ├────────┼──────────────────────┼─────────────────────────┤
    │  0x17  │ [end of frame]       │ total length = 23 bytes │
    └────────┴──────────────────────┴─────────────────────────┘
*/

#include <stdint.h>
#include <sys/types.h>

#include "commands.h"

#define REQ_MAX       65535u
#define RSP_MAX       65535u

#define TLV_HDR_SZ    ((size_t)sizeof(tlv_t))


/* ------------------------------------------------------------------ *
 *  Wire-format structs   (packed = no padding)                       *
 * ------------------------------------------------------------------ */

typedef struct __attribute__((packed)) {
    uint8_t  type;                     /* value type        */
    uint16_t len;                      /* value byte lenght */
    /* value follows immediately after this */
} tlv_t;

typedef struct __attribute__((packed)) {
    uint16_t len;                      /* header + TLVs      */
    uint8_t  opcode;                   /* command identifier */
    pid_t    pid;                      /* sender PID         */
} req_hdr_t;

typedef struct __attribute__((packed)) {
    uint16_t len;                      /* header + TLVs          */
    uint8_t  opcode;                   /* echo of request opcode */
    uint8_t  status;                   /* 0 = OK, else errno     */
} rsp_hdr_t;


/* Full frame = hdr + TLVs */

typedef struct __attribute__((packed)) {
    req_hdr_t hdr;
    uint8_t   payload[REQ_MAX - sizeof(req_hdr_t)];
} request_t;

typedef struct __attribute__((packed)) {
    rsp_hdr_t hdr;
    uint8_t   payload[RSP_MAX - sizeof(rsp_hdr_t)];
} response_t;


/* === Decoder builder ============================================= */
typedef struct {
    uint8_t *buf;     /* points inside request_t.payload[] */
    size_t   cap;     /* bytes available                   */
    size_t   used;    /* bytes already filled              */
} proto_builder_t;

typedef struct {
    uint8_t     type;
    uint16_t    len;
    const void *ptr;
} arg_t;

/* === Request builders ============================================ */
int proto_req_init   (request_t  *req, proto_builder_t *b, uint8_t opcode);
int proto_add_tlv    (proto_builder_t *b, uint8_t type,
                      const void *val, size_t len);
int proto_req_finish (request_t  *req, const proto_builder_t *b);

/* === Response builders =========================================== */
int proto_rsp_init   (response_t *rsp, proto_builder_t *b,
                      uint8_t opcode, uint8_t status);
int proto_rsp_finish (response_t *rsp, const proto_builder_t *b);


/* === Decoder cursor ============================================== */
typedef struct {
    const uint8_t *cur, *end;
} proto_cursor_t;

/* === Decoder walker ============================================== */
void proto_cursor_init(proto_cursor_t *, const uint8_t *payload, size_t len);
int  proto_cursor_next(proto_cursor_t *, uint8_t *type,
                       const void **val, uint16_t *len);

/* === I/O helpers  ================================================ */
int proto_send_req (int fd, const request_t *req);
int proto_recv_req (int fd,       request_t *out);

int proto_send_rsp (int fd, const response_t *rsp);
int proto_recv_rsp (int fd,       response_t *out);

void proto_build_simple_rsp (response_t *rsp, uint8_t op, const char *msg);
int  proto_arg_first_str    (const request_t *req, char *dst, size_t cap);

#endif /* PROTOCOL_H */
