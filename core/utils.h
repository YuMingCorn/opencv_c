#ifndef UTILS_H_
#define UTILS_H_

#include "stddef.h"
#include "stdlib.h"

#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "Assertion failed: %s, file: %s:%d\n", #expr,      \
                    __FILE__, __LINE__);                                       \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

void *fast_malloc(size_t size);
void  fast_free(void *ptr);

#endif // UTILS_H_