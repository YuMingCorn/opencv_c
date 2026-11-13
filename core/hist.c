#include "hist.h"

#include <string.h>
static void check_u8c1(const struct Mat *m) {
    ASSERT(m != NULL);
    ASSERT(mat_type(m) == (size_t)CV_8UC1);
    ASSERT(m->rows >= 0 && m->cols >= 0);
    ASSERT((uint64_t)(uint32_t)m->rows * (uint64_t)(uint32_t)m->cols <=
           (uint64_t)HIST_MAX_PIXELS);
}

void calc_hist(uint32_t hist[256], const struct Mat *src) {
    ASSERT(hist != NULL);
    check_u8c1(src);
    memset(hist, 0, 256 * sizeof hist[0]);
    for (int y = 0; y < src->rows; y++) {
        const uchar *row = mat_row(src, y);
        for (int x = 0; x < src->cols; x++) {
            hist[row[x]]++;
        }
    }
}
