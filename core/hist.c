#include "hist.h"

#include <string.h>

#include "pixel.h"

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

void equalize_hist(struct Mat *dst, const struct Mat *src) {
    check_u8c1(src);
    ASSERT(dst != NULL);
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(mat_type(dst) == (size_t)CV_8UC1);
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);
    if (src->rows == 0 || src->cols == 0) {
        return;
    }

    // Scratch: histogram (1024 bytes) followed by the LUT (256 bytes).
    uint32_t *hist = fast_malloc(HIST_EQUALIZE_SCRATCH);
    ASSERT(hist != NULL);
    uchar *lut = (uchar *)(hist + 256);
    calc_hist(hist, src);

    uint32_t total = (uint32_t)src->rows * (uint32_t)src->cols;
    int      m     = 0;
    while (hist[m] == 0) {
        m++;
    }
    uint32_t den = total - hist[m];
    if (den == 0) {
        for (int i = 0; i < 256; i++) {
            lut[i] = (uchar)i;
        }
    } else {
        uint32_t sum = 0;
        for (int v = 0; v <= m; v++) {
            lut[v] = 0;
        }
        for (int v = m + 1; v < 256; v++) {
            sum += hist[v];
            // sum <= 2^24 - 1, so sum * 255 + den / 2 < 2^32.
            lut[v] = (uchar)((sum * 255u + den / 2u) / den);
        }
    }
    mat_lut(dst, src, lut);
    fast_free(hist);
}
