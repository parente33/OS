/**
 * @file protocol.c
 * @brief Implementation of the binary TLV protocol
 */

#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "transport.h"
#include "status.h"
#include "logger.h"

/* Internal helpers */

/* Check if total size exceeds capacity */
static inline int
check_cap(size_t total, size_t cap)
{
    if (total > cap) {
        util_error("Message size %zu exceeds maximum capacity %zu\n", total, cap);
        return OS_ERROR;
    }
    return OS_OK;
}

/* Request Builder Functions */

int
proto_req_init(request_t *req, proto_builder_t *b, uint8_t op)
{
    if (!req || !b) {
        util_error("Invalid request or builder pointer\n");
        return OS_ERROR;
    }

    memset(req, 0, sizeof *req);
    req->hdr.opcode = op;
    req->hdr.pid    = getpid();

    b->buf  = req->payload;
    b->used = 0;
    b->cap  = sizeof req->payload;

    return OS_OK;
}

/* Response Builder Functions */

int
proto_rsp_init(response_t *rsp, proto_builder_t *b,
               uint8_t op, uint8_t status)
{
    if (!rsp || !b) {
        util_error("Invalid response or builder pointer\n");
        return OS_ERROR;
    }

    memset(rsp, 0, sizeof *rsp);
    rsp->hdr.opcode = op;
    rsp->hdr.status = status;

    b->buf  = rsp->payload;
    b->used = 0;
    b->cap  = sizeof rsp->payload;

    return OS_OK;
}

/* TLV Operations */

int
proto_add_tlv(proto_builder_t *b, uint8_t type,
              const void *val, size_t len)
{
    if (!b || !val) {
        util_error("Invalid builder or value pointer\n");
        return OS_ERROR;
    }

    if (len > UINT16_MAX) {
        util_error("TLV value length %zu exceeds maximum %u\n",
                  len, UINT16_MAX);
        return OS_ERROR;
    }

    size_t need = TLV_HDR_SZ + len;
    if (b->used + need > b->cap) {
        util_error("TLV would exceed payload capacity\n");
        return OS_ERROR;
    }

    tlv_t h = {
        .type = type,
        .len  = (uint16_t)len
    };

    memcpy(b->buf + b->used,              &h,  TLV_HDR_SZ);
    memcpy(b->buf + b->used + TLV_HDR_SZ, val, len);

    b->used += need;
    return OS_OK;
}

/* Message Finalization */

int
proto_req_finish(request_t *req, const proto_builder_t *b)
{
    if (!req || !b) {
        util_error("Invalid request or builder pointer\n");
        return OS_ERROR;
    }

    size_t total = sizeof req->hdr + b->used;
    if (check_cap(total, REQ_MAX) == OS_ERROR)
        return OS_ERROR;

    req->hdr.len = (uint16_t)total;
    return OS_OK;
}

int
proto_rsp_finish(response_t *rsp, const proto_builder_t *b)
{
    if (!rsp || !b) {
        util_error("Invalid response or builder pointer\n");
        return OS_ERROR;
    }

    size_t total = sizeof rsp->hdr + b->used;
    if (check_cap(total, RSP_MAX) == OS_ERROR)
        return OS_ERROR;

    rsp->hdr.len = (uint16_t)total;
    return OS_OK;
}

/* TLV Decoder */

void
proto_cursor_init(proto_cursor_t *c, const uint8_t *buf, size_t len)
{
    if (!c || !buf) {
        util_error("Invalid cursor or buffer pointer\n");
        return;
    }
    c->cur = buf;
    c->end = buf + len;
}

int
proto_cursor_next(proto_cursor_t *c, uint8_t *type,
                  const void **val, uint16_t *vlen)
{
    if (!c || !type || !val || !vlen) {
        util_error("Invalid cursor or output pointer\n");
        return OS_ERROR;
    }

    if (c->cur + TLV_HDR_SZ > c->end) {
        return OS_OK;        /* No more TLVs */
    }

    const tlv_t *t = (const tlv_t *)c->cur;
    size_t need = TLV_HDR_SZ + t->len;

    if (c->cur + need > c->end) {
        util_error("Corrupt TLV: would read past end of buffer\n");
        return OS_ERROR;
    }

    *type = t->type;
    *val  = c->cur + TLV_HDR_SZ;
    *vlen = t->len;

    c->cur += need;
    return OS_AGAIN;
}

/* I/O Functions */

int
proto_recv_req(int fd, request_t *out)
{
    if (!out) {
        util_error("Invalid output pointer\n");
        return OS_ERROR;
    }

    req_hdr_t hdr;
    if (txp_read(fd, &hdr, sizeof hdr) == OS_ERROR) {
        util_error("Failed to read request header: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (hdr.len < sizeof hdr || hdr.len > REQ_MAX) {
        util_error("Invalid request length: %u\n", hdr.len);
        return OS_ERROR;
    }

    size_t payload = hdr.len - sizeof hdr;
    uint8_t buf[REQ_MAX];

    memcpy(buf, &hdr, sizeof hdr);
    if (txp_read(fd, buf + sizeof hdr, payload) == OS_ERROR) {
        util_error("Failed to read request payload: %s\n", strerror(errno));
        return OS_ERROR;
    }

    memcpy(out, buf, hdr.len);
    return OS_OK;
}

int
proto_send_req(int fd, const request_t *req)
{
    if (!req) {
        util_error("Invalid request pointer\n");
        return OS_ERROR;
    }
    return txp_write(fd, req, req->hdr.len);
}

int
proto_send_rsp(int fd, const response_t *rsp)
{
    if (!rsp) {
        util_error("Invalid response pointer\n");
        return OS_ERROR;
    }
    return txp_write(fd, rsp, rsp->hdr.len);
}

int
proto_recv_rsp(int fd, response_t *out)
{
    if (!out) {
        util_error("Invalid output pointer\n");
        return OS_ERROR;
    }

    rsp_hdr_t hdr;
    if (txp_read(fd, &hdr, sizeof hdr) == OS_ERROR) {
        util_error("Failed to read response header: %s\n", strerror(errno));
        return OS_ERROR;
    }

    if (hdr.len < sizeof hdr || hdr.len > RSP_MAX) {
        util_error("Invalid response length: %u\n", hdr.len);
        return OS_ERROR;
    }

    size_t payload = hdr.len - sizeof hdr;
    uint8_t buf[RSP_MAX];

    memcpy(buf, &hdr, sizeof hdr);
    if (txp_read(fd, buf + sizeof hdr, payload) == OS_ERROR) {
        util_error("Failed to read response payload: %s\n", strerror(errno));
        return OS_ERROR;
    }

    memcpy(out, buf, hdr.len);
    return OS_OK;
}

void
proto_build_simple_rsp(response_t *rsp, uint8_t op, const char *msg)
{
    if (!rsp) {
        util_error("Invalid response pointer\n");
        return;
    }

    proto_builder_t b = {0};
    proto_rsp_init(rsp, &b, op, 0);

    if (msg) {
        proto_add_tlv(&b, ARG_STR, msg, strlen(msg));
    }

    proto_rsp_finish(rsp, &b);
}

int
proto_arg_first_str(const request_t *req, char *dst, size_t cap)
{
    if (!req || !dst) {
        util_error("Invalid request or destination pointer\n");
        return OS_ERROR;
    }

    /* Initialize cursor over the payload */
    proto_cursor_t cur;
    proto_cursor_init(&cur,
                      req->payload,
                      req->hdr.len - sizeof req->hdr);

    /* Fetch the first TLV */
    uint8_t  t;
    const void *v;
    uint16_t l;
    int st = proto_cursor_next(&cur, &t, &v, &l);

    if (st != OS_AGAIN ||           /* No TLVs or corruption */
        t != ARG_STR ||             /* First arg not a string */
        l == 0 ||                   /* Empty keyword */
        l >= cap) {                 /* Won't fit in destination */
        util_error("Invalid or missing string argument\n");
        return OS_ERROR;
    }

    /* Copy and NUL-terminate */
    memcpy(dst, v, l);
    dst[l] = '\0';

    return OS_OK;
}
