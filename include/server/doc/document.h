#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stdint.h>

#define MAX_TITLE_LEN        200
#define MAX_AUTHORS_LEN      200
#define MAX_PATH_LEN         64

typedef struct {
    int      key;
    char     title   [MAX_TITLE_LEN];
    char     authors [MAX_AUTHORS_LEN];
    char     path    [MAX_PATH_LEN];
    uint32_t year;
} document_t;

#endif /* DOCUMENT_H */