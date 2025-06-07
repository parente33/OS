#include <stdio.h>
#include <string.h>

#include "document.h"
#include "storage.h"
#include "handlers.h"
#include "protocol.h"
#include "status.h"

int
handle_a(const arg_val_t argv[], response_t *rsp)
{
    document_t  doc;
    int         title_len, auth_len, path_len;

    /* copy title */
    title_len = (argv[0].v.str.len < MAX_TITLE_LEN) ? (int)argv[0].v.str.len : (MAX_TITLE_LEN - 1);
    memcpy(doc.title, argv[0].v.str.ptr, title_len); doc.title[title_len] = '\0';

    /* copy authors */
    auth_len = (argv[1].v.str.len < MAX_AUTHORS_LEN) ? (int)argv[1].v.str.len : (MAX_AUTHORS_LEN - 1);
    memcpy(doc.authors, argv[1].v.str.ptr, auth_len); doc.authors[auth_len] = '\0';

    /* copy year */
    doc.year = argv[2].v.u32;

    /* copy path */
    path_len = (argv[3].v.str.len < MAX_PATH_LEN) ? (int)argv[3].v.str.len : (MAX_PATH_LEN - 1);
    memcpy(doc.path, argv[3].v.str.ptr, path_len); doc.path[path_len] = '\0';

    /* assign a key */
    int key = stg_add_doc(&doc);

    if (key == OS_ERROR)
        return OS_ERROR;

    doc.key = key; /* for potential debugging */

    char msg[64];
    snprintf(msg, sizeof msg, "Document %d indexed", key);
    proto_build_simple_rsp(rsp, OP_A, msg);

    return OS_OK;
}
