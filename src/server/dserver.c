#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "storage.h"
#include "commands.h"
#include "protocol.h"
#include "arg_codec.h"
#include "transport.h"
#include "handlers.h"
#include "dispatcher.h"
#include "status.h"
#include "logger.h"
#include "docroot.h"
#include "cache.h"

static void reap_zombies(void)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;  /* collect finished children */
}

/* ------------------------------------------------------------------ */
/* fork a worker, give it one pipe end, return the read end to parent  */
static pid_t
spawn_nonblock_child(const request_t *req,
                     const cmd_row_t *cmd,
                     int *out_pipe_rd)
{
    int pfd[2];                             /* pfd[0]=read, pfd[1]=write */
    if (pipe(pfd) < 0) return -1;

    pid_t pid = fork();
    if (pid < 0) {                          /* fork error */
        close(pfd[0]); close(pfd[1]);
        return -1;
    }

    if (pid == 0)                           /* ---------- child ---------- */
    {
        close(pfd[0]);                      /* keep write end only */

        response_t rsp;
        int rc = dispatch_request(req, cmd, &rsp);

        if (rc == OS_ERROR)                 /* guarantee a reply frame   */
            proto_build_simple_rsp(&rsp, cmd->opcode, "ERR");

        (void)proto_send_rsp(pfd[1], &rsp); /* hand back to parent       */
        close(pfd[1]);
        _exit(rc == OS_SHUTDOWN ? 0 : 1);
    }

    /* ----------------------------- parent ----------------------------- */
    close(pfd[1]);                          /* keep read end only */
    *out_pipe_rd = pfd[0];
    return pid;
}

/* ------------------------------------------------------------------ */
static int
serve_requests(txp_t *xp)
{
    for ( ; ; )
    {
        reap_zombies();

        request_t req;
        if (proto_recv_req(xp->in_fd, &req) == OS_ERROR)
            continue;

        const cmd_row_t *cmd = cmd_by_opcode(req.hdr.opcode);
        if (!cmd) continue;

        /* ---------- QUICK cache HIT check before forking ------------- */
        if (req.hdr.opcode == OP_S)
        {
            char kw[256];
            if (proto_arg_first_str(&req, kw, sizeof kw) == OS_OK)
            {
                response_t hit;
                if (cache_get(kw, &hit) == 0) {
                    txp_reply(req.hdr.pid, &hit, hit.hdr.len);
                    continue;               /* hit → no fork            */
                }
            }
        }

        /* ---------- NON-blocking commands run in a worker ------------ */
        if (!cmd->blocking)
        {
            int pipe_rd;
            if (spawn_nonblock_child(&req, cmd, &pipe_rd) < 0)
                continue;                   /* fork failure: ignore      */

            response_t rsp;
            if (proto_recv_rsp(pipe_rd, &rsp) == OS_OK)
            {
                if (req.hdr.opcode == OP_S)
                {
                    char kw[256];
                    if (proto_arg_first_str(&req, kw, sizeof kw) == OS_OK)
                        cache_put(kw, &rsp);/* single-writer insert      */
                }
                txp_reply(req.hdr.pid, &rsp, rsp.hdr.len);
            }
            close(pipe_rd);
            continue;                       /* listen for next request   */
        }

        /* ---------- blocking command handled in parent --------------- */
        response_t rsp;
        int rc = dispatch_request(&req, cmd, &rsp);
        if (rc != OS_ERROR)
            txp_reply(req.hdr.pid, &rsp, rsp.hdr.len);

        if (rc == OS_SHUTDOWN)
            return OS_SHUTDOWN;
    }
}

/* ------------------------------------------------------------------ */
int
main(int argc, char *argv[])
{
    if (argc != 3) {
        util_error("Usage: %s <document_folder> <cache_size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (docroot_set(argv[1]) == OS_ERROR) {
        util_error("Invalid document folder path.\n");
        return EXIT_FAILURE;
    }

    if (stg_init() == OS_ERROR)
        return EXIT_FAILURE;

    size_t cache_cap = (size_t)atoi(argv[2]);
    cache_init(cache_cap);

    txp_t xp;
    if (txp_open_server(&xp) == OS_ERROR) {
        perror("txp_open_server failed");
        return EXIT_FAILURE;
    }

    if (serve_requests(&xp) == OS_SHUTDOWN) {
        cache_cleanup();
        reap_zombies();
        stg_close();
        txp_close(&xp);
    }
    return EXIT_SUCCESS;
}
