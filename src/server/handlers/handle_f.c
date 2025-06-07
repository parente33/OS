#include <stdio.h>

#include "handlers.h"
#include "protocol.h"
#include "status.h"

int handle_f(const arg_val_t argv[], response_t *rsp)
{
    (void) *argv;

    const char msg[] = "Server is shutting down";
    proto_build_simple_rsp(rsp, OP_F, msg);

    /* tell dispatcher/main to stop the server loop */
    return OS_SHUTDOWN;
}