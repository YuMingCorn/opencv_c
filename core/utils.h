#ifndef UTILS_H_
#define UTILS_H_

#include <stddef.h>
#include <stdlib.h>

#ifdef IMPROC_NO_STDIO
#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            abort();                                                           \
        }                                                                      \
    } while (0)
#else
#include <stdio.h>
#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "Assertion failed: %s, file: %s:%d\n", #expr,      \
                    __FILE__, __LINE__);                                       \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)
#endif

void *fast_malloc(size_t size);
void  fast_free(void *ptr);

#endif // UTILS_H_
