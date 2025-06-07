#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#include "document.h"
#include "storage.h"
#include "handlers.h"
#include "protocol.h"
#include "status.h"
#include "logger.h"
#include "docutil.h"

#define BITSET_BYTE(i)   ((i) >> 3)
#define BITSET_MASK(i)   (1u << ((i) & 7))

#define LIST_CAP         (RSP_MAX - TLV_HDR_SZ)

static int
scan_parallel(const char *kw, int total, uint32_t workers, uint8_t *bmp)
{
    if (workers == 0) workers = 1;

    /* cap workers to avoid oversubscription */
    long cpus = get_nprocs();                    /* portable CPU count  */
    if (workers > (uint32_t)(cpus * 10)) workers = (uint32_t)(cpus * 10);
    if (workers > (uint32_t)total)       workers = (uint32_t)total;

    /* shared atomic counter */
    int *next_key = mmap(NULL, sizeof *next_key,
                         PROT_READ  |  PROT_WRITE,
                         MAP_SHARED |  MAP_ANONYMOUS, -1, 0);
    if (next_key == MAP_FAILED) return OS_ERROR;

    *next_key = 0;
    pid_t pids[workers];

    for (uint32_t w = 0; w < workers; ++w)
    {
        pid_t pid = fork();
        if (pid < 0) { util_error("fork\n"); return OS_ERROR; }

        if (pid == 0)
        {                    /* ---- child ---- */
            for ( ; ; )
            {
                int k = __sync_fetch_and_add(next_key, 1);
                if (k >= total) break;

                if (doc_key_contains_keyword(k, kw) == 1) {
                    bmp[BITSET_BYTE(k)] |= BITSET_MASK(k);
                }
            }
            _exit(0);
        }
        pids[w] = pid;                     /* parent  */
    }

    for (uint32_t w = 0; w < workers; ++w)  /* wait all */
        waitpid(pids[w], NULL, 0);

    munmap(next_key, sizeof *next_key);

    return OS_OK;
}

int
handle_s(const arg_val_t argv[], response_t *rsp)
{
    /* parse arguments */
    char kw[256];
    size_t kw_len = argv[0].v.str.len;

    if (kw_len >= sizeof kw) kw_len = sizeof kw - 1;
    memcpy(kw, argv[0].v.str.ptr, kw_len);
    kw[kw_len] = '\0';

    uint32_t workers = 1;
    if (argv[1].type == ARG_U32 && argv[1].v.u32)
        workers = argv[1].v.u32;

    /* number of docs */
    int total = stg_total();
    if (total <= 0) return OS_ERROR;

    /* shared bitmap (1 bit per key) */
    size_t bmp_bytes = (size_t)(total + 7) / 8;
    uint8_t *bmp = mmap(NULL, bmp_bytes,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (bmp == MAP_FAILED) return OS_ERROR;
    memset(bmp, 0, bmp_bytes);

    /* parallel scan */
    if (scan_parallel(kw, total, workers, bmp) == OS_ERROR) {
        munmap(bmp, bmp_bytes);
        return OS_ERROR;
    }

    /* build "[k1, k2, …]" string */
    char list_buf[LIST_CAP]; size_t pos = 0;
    pos += sprintf(list_buf + pos, "[");

    int first = 1;
    for (int k = 0; k < total; ++k)
    {
        if (bmp[BITSET_BYTE(k)] & BITSET_MASK(k)) {
            pos += sprintf(list_buf + pos, "%s%d", first ? "" : ", ", k);
            first = 0;
        }
    }
    sprintf(list_buf + pos, "]");

    munmap(bmp, bmp_bytes);

    proto_build_simple_rsp(rsp, OP_S, list_buf);

    return OS_OK;
}
