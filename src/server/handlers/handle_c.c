#include <stdio.h>
#include <string.h>

#include "document.h"
#include "storage.h"
#include "handlers.h"
#include "protocol.h"
#include "status.h"

int
handle_c(const arg_val_t argv[], response_t *rsp)
{
    /* The Consult command receives the key of the document we want to fetch. */
    uint32_t key_u32 = argv[0].v.u32;
    int      key     = (int)key_u32;

    document_t doc;
    if (stg_get_doc(key, &doc) == OS_ERROR)
    {
        /* Not found – answer with a simple message so the client still
         * receives a response frame (dispatch ignores OS_ERROR).
         */
        const char msg[] = "Document not found";
        proto_build_simple_rsp(rsp, OP_C, msg);

        return OS_OK;
    }

    /* Build a multi-TLV response with the document fields. */
    char line[256];
    proto_builder_t b = {0};

    proto_rsp_init (rsp, &b, OP_C, 0);

    snprintf(line, sizeof line, "Title: %s", doc.title);
    proto_add_tlv(&b, ARG_STR, line, strlen(line));

    snprintf(line, sizeof line, "Authors: %s", doc.authors);
    proto_add_tlv(&b, ARG_STR, line, strlen(line));

    snprintf(line, sizeof line, "Year: %u", doc.year);
    proto_add_tlv(&b, ARG_STR, line, strlen(line));

    snprintf(line, sizeof line, "Path: %s", doc.path);
    proto_add_tlv(&b, ARG_STR, line, strlen(line));

    proto_rsp_finish(rsp, &b);

    return OS_OK;
}