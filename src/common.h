/* DannyNiu/NJF, 2023-06-30. Public Domain. */

#ifndef dcc_common_h
#define dcc_common_h 1

#ifndef __has_include
#define __has_include(...) 0
#endif /* __has_include */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct logging_ctxbase;
typedef int (*logger_func)(struct logging_ctxbase *, const char *);

struct logging_ctxbase {
    logger_func logger;
    void *aux;
};

#if '0' != 0x30 || 'A' != 0x41
#error Non-ASCII environments need adaptation!
#endif /* Testing for ASCII character set. */

#endif /* dcc_common_h */
