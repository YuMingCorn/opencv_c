#include <string.h>

#include "geometry.h"
#include "utils.h"

static void flip_h_row(const uchar *restrict s, uchar *restrict d, int width,
                       int cn) {
    const uchar *p = s + (size_t)(width - 1) * (size_t)cn;
    for (int x = 0; x < width; x++, p -= cn, d += cn) {
        for (int c = 0; c < cn; c++) {
            d[c] = p[c];
        }
    }
}

void flip(struct Mat *dst, const struct Mat *src, int flip_code) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(flip_code == FLIP_VERTICAL || flip_code == FLIP_HORIZONTAL ||
           flip_code == FLIP_BOTH);
    ASSERT(mat_depth(src) == CV_8U);
    ASSERT(mat_channel_num(src) == 1 || mat_channel_num(src) == 3 ||
           mat_channel_num(src) == 4);
    ASSERT(mat_type(dst) == mat_type(src));
    ASSERT(src->rows == dst->rows && src->cols == dst->cols);
    if (src->rows == 0 || src->cols == 0) {
        return;
    }
    int    cn         = (int)mat_channel_num(src);
    size_t row_bytes  = (size_t)src->cols * (size_t)cn;
    int    horizontal = flip_code != FLIP_VERTICAL;
    int    vertical   = flip_code != FLIP_HORIZONTAL;
    for (int y = 0; y < src->rows; y++) {
        const uchar *s = mat_row(src, vertical ? src->rows - 1 - y : y);
        uchar       *d = mat_row(dst, y);
        if (horizontal) {
            flip_h_row(s, d, src->cols, cn);
        } else {
            memcpy(d, s, row_bytes);
        }
    }
}
