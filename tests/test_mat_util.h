#ifndef TEST_MAT_UTIL_H_
#define TEST_MAT_UTIL_H_

#include <string.h>

#include "mat.h"
#include "test_util.h"

static inline struct Mat *mat_from_bytes(int rows, int cols, int type,
                                         const void *bytes) {
    struct Mat *m = alloc_mat(rows, cols, type);
    memcpy(m->data, bytes, (size_t)rows * m->step[0]);
    return m;
}

static inline void fill_pattern(struct Mat *m, unsigned seed) {
    size_t n = (size_t)m->rows * m->step[0];
    for (size_t i = 0; i < n; i++) {
        m->data[i] = (uchar)(i * 37u + seed);
    }
}

#define CHECK_MAT_BYTES(mat, expected)                                         \
    CHECK_EQ_BYTES((mat)->data, (expected), (size_t)(mat)->rows *(mat)->step[0])

#endif // TEST_MAT_UTIL_H_
