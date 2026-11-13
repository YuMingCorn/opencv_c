#include <stdint.h>
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

static void resize_nearest_row(const uchar *restrict s, uchar *restrict d,
                               int dst_w, int cn, const int *sx) {
    for (int x = 0; x < dst_w; x++, d += cn) {
        const uchar *p = s + (size_t)sx[x] * (size_t)cn;
        for (int c = 0; c < cn; c++) {
            d[c] = p[c];
        }
    }
}

static void resize_linear_row(const uchar *restrict r0,
                              const uchar *restrict r1, uchar *restrict d,
                              int dst_w, int cn, const int *x0, const int *x1,
                              const uint16_t *fx, int fy) {
    for (int x = 0; x < dst_w; x++, d += cn) {
        const uchar *a0 = r0 + (size_t)x0[x] * (size_t)cn;
        const uchar *a1 = r0 + (size_t)x1[x] * (size_t)cn;
        const uchar *b0 = r1 + (size_t)x0[x] * (size_t)cn;
        const uchar *b1 = r1 + (size_t)x1[x] * (size_t)cn;
        uint32_t     f  = fx[x];
        for (int c = 0; c < cn; c++) {
            uint32_t top = a0[c] * (256u - f) + a1[c] * f;
            uint32_t bot = b0[c] * (256u - f) + b1[c] * f;
            d[c] = (uchar)((top * (256u - (uint32_t)fy) + bot * (uint32_t)fy +
                            32768u) >>
                           16);
        }
    }
}

// Half-pixel centre mapping of destination index dx to source position, in
// Q8 as (x0, fx) with x1 = min(x0 + 1, sw - 1).
static void linear_coord(int dx, int sw, int dw, int *x0, uint16_t *fx) {
    int64_t n = (int64_t)(2 * (int64_t)dx + 1) * sw - dw;
    int64_t d = 2 * (int64_t)dw;
    int64_t i = 0;
    int64_t f = 0;
    if (n >= 0) {
        i = n / d;
        f = ((n - i * d) * 256 + dw) / d;
        if (i >= sw - 1) {
            i = sw - 1;
            f = 0;
        }
    }
    *x0 = (int)i;
    *fx = (uint16_t)f;
}

void resize(struct Mat *dst, const struct Mat *src, int interpolation) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(interpolation == INTER_NEAREST || interpolation == INTER_LINEAR);
    ASSERT(mat_depth(src) == CV_8U);
    ASSERT(mat_channel_num(src) == 1 || mat_channel_num(src) == 3 ||
           mat_channel_num(src) == 4);
    ASSERT(mat_type(dst) == mat_type(src));
    if (dst->rows == 0 || dst->cols == 0) {
        return;
    }
    ASSERT(src->rows > 0 && src->cols > 0);

    int dw = dst->cols, dh = dst->rows, sw = src->cols, sh = src->rows;
    int cn = (int)mat_channel_num(src);

    int      *xs  = fast_malloc(2 * (size_t)dw * sizeof(int));
    uint16_t *fxs = fast_malloc((size_t)dw * sizeof(uint16_t));
    ASSERT(xs != NULL && fxs != NULL);
    int *x0 = xs, *x1 = xs + dw;

    if (interpolation == INTER_NEAREST) {
        for (int x = 0; x < dw; x++) {
            x0[x] =
                (int)(((int64_t)(2 * (int64_t)x + 1) * sw) / (2 * (int64_t)dw));
        }
        for (int y = 0; y < dh; y++) {
            int sy =
                (int)(((int64_t)(2 * (int64_t)y + 1) * sh) / (2 * (int64_t)dh));
            resize_nearest_row(mat_row(src, sy), mat_row(dst, y), dw, cn, x0);
        }
    } else {
        for (int x = 0; x < dw; x++) {
            linear_coord(x, sw, dw, &x0[x], &fxs[x]);
            x1[x] = x0[x] + 1 < sw ? x0[x] + 1 : sw - 1;
        }
        for (int y = 0; y < dh; y++) {
            int      y0;
            uint16_t fy;
            linear_coord(y, sh, dh, &y0, &fy);
            int y1 = y0 + 1 < sh ? y0 + 1 : sh - 1;
            resize_linear_row(mat_row(src, y0), mat_row(src, y1),
                              mat_row(dst, y), dw, cn, x0, x1, fxs, fy);
        }
    }
    fast_free(xs);
    fast_free(fxs);
}
