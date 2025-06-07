/**
 * @file logger.c
 * @brief Implementation of logging utilities
 */

#include "logger.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/** Maximum size of the print buffer */
#define UTIL_PRINT_BUFFER_SIZE 65535u

void util_log(const char *format, ...)
{
    char buf[UTIL_PRINT_BUFFER_SIZE];

    va_list list;
    va_start(list, format);

    (void) !write(STDOUT_FILENO, buf, vsnprintf(buf, UTIL_PRINT_BUFFER_SIZE, format, list));

    va_end(list);
}

void util_error(const char *format, ...)
{
    char buf[UTIL_PRINT_BUFFER_SIZE];
    va_list list;
    va_start(list, format);

    (void) !write(STDERR_FILENO, buf, vsnprintf(buf, UTIL_PRINT_BUFFER_SIZE, format, list));

    va_end(list);
}
