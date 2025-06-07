#include <stdio.h>

#include "storage.h"
#include "handlers.h"
#include "protocol.h"
#include "status.h"

int
handle_d(const arg_val_t argv[], response_t *rsp)
{
    uint32_t key_u32 = argv[0].v.u32;
    int      key     = (int)key_u32;

    int rc = stg_del_doc(key);

    char msg[64];
    if (rc == OS_OK)
        snprintf(msg, sizeof msg, "Index entry %d deleted", key);
    else
        snprintf(msg, sizeof msg, "Index entry %d not found", key);

    proto_build_simple_rsp(rsp, OP_D, msg);

    return OS_OK;
}