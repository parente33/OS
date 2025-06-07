#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#include "docutil.h"
#include "document.h"
#include "storage.h"
#include "handlers.h"
#include "protocol.h"
#include "logger.h"
#include "status.h"

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

int
handle_l(const arg_val_t argv[], response_t *rsp)
{
    uint32_t key_u32 = argv[0].v.u32;
    int      key     = (int)key_u32;

    /* Extract keyword; ensure null terminator */
    char kw[256];
    size_t kw_len = argv[1].v.str.len;
    if (kw_len >= sizeof kw) kw_len = sizeof kw - 1;

    memcpy(kw, argv[1].v.str.ptr, kw_len);
    kw[kw_len] = '\0';

    /* Fetch document metadata to get relative path */
    document_t doc;
    if (stg_get_doc(key, &doc) == OS_ERROR) {
        proto_build_simple_rsp(rsp, OP_L, "Document not found");
        return OS_OK;
    }

    /* Build full path: <docroot>/<doc.path> */
    char fullpath[PATH_MAX];
    if (doc_build_path(key, fullpath, sizeof fullpath) == OS_ERROR) {
        proto_build_simple_rsp(rsp, OP_L, "Path not found");
        return OS_OK;
    }

    size_t count = 0;
    if (doc_count_keyword(fullpath, kw, 0, &count) == OS_ERROR) {
        util_error("Parsing error\n");
        return OS_ERROR;
    }

    proto_builder_t b = {0};
    proto_rsp_init(rsp, &b, OP_L, 0);

    uint32_t count_u32 = (uint32_t)count;
    proto_add_tlv(&b, ARG_U32, &count_u32, sizeof count_u32);

    proto_rsp_finish(rsp, &b);
    
    return OS_OK;
}