#include "morph.h"

#include <stdint.h>

#include "utils.h"

static int is_u8_image_type(const struct Mat *m) {
    size_t t = mat_type(m);
    return t == (size_t)CV_8UC1 || t == (size_t)CV_8UC3 || t == (size_t)CV_8UC4;
}

// acc[i] = min/max(acc[i], row[i]) for n bytes.
static void vmin_row(uchar *restrict acc, const uchar *restrict row, int n) {
    for (int i = 0; i < n; i++) {
        acc[i] = row[i] < acc[i] ? row[i] : acc[i];
    }
}

static void vmax_row(uchar *restrict acc, const uchar *restrict row, int n) {
    for (int i = 0; i < n; i++) {
        acc[i] = row[i] > acc[i] ? row[i] : acc[i];
    }
}

// Horizontal pass: out[x] = min/max of in[x - r .. x + r], clipped to
// [0, cols - 1], per channel. in and out hold cols * cn bytes and are distinct.
static void hmin_row(const uchar *restrict in, uchar *restrict out, int cols,
                     int cn, int r) {
    for (int x = 0; x < cols; x++) {
        const int lo = x - r < 0 ? 0 : x - r;
        const int hi = x + r > cols - 1 ? cols - 1 : x + r;
        for (int c = 0; c < cn; c++) {
            uchar m = in[lo * cn + c];
            for (int i = lo + 1; i <= hi; i++) {
                uchar v = in[i * cn + c];
                m       = v < m ? v : m;
            }
            out[x * cn + c] = m;
        }
    }
}

static void hmax_row(const uchar *restrict in, uchar *restrict out, int cols,
                     int cn, int r) {
    for (int x = 0; x < cols; x++) {
        const int lo = x - r < 0 ? 0 : x - r;
        const int hi = x + r > cols - 1 ? cols - 1 : x + r;
        for (int c = 0; c < cn; c++) {
            uchar m = in[lo * cn + c];
            for (int i = lo + 1; i <= hi; i++) {
                uchar v = in[i * cn + c];
                m       = v > m ? v : m;
            }
            out[x * cn + c] = m;
        }
    }
}

static void morph(struct Mat *dst, const struct Mat *src, int ksize,
                  int is_dilate) {
    ASSERT(dst != NULL && src != NULL);
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(is_u8_image_type(src));
    ASSERT(mat_type(dst) == mat_type(src));
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);
    ASSERT(ksize >= 1 && ksize <= 255 && (ksize & 1));
    if (src->rows == 0 || src->cols == 0) {
        return;
    }

    const int rows = src->rows;
    const int cn   = (int)mat_channel_num(src);
    const int n    = src->cols * cn;
    const int r    = ksize / 2;

    uchar *buf = fast_malloc((size_t)n);
    ASSERT(buf != NULL);

    for (int y = 0; y < rows; y++) {
        const int y0 = y - r < 0 ? 0 : y - r;
        const int y1 = y + r > rows - 1 ? rows - 1 : y + r;
        for (int i = 0; i < n; i++) {
            buf[i] = mat_row(src, y0)[i];
        }
        for (int yy = y0 + 1; yy <= y1; yy++) {
            if (is_dilate) {
                vmax_row(buf, mat_row(src, yy), n);
            } else {
                vmin_row(buf, mat_row(src, yy), n);
            }
        }
        if (is_dilate) {
            hmax_row(buf, mat_row(dst, y), src->cols, cn, r);
        } else {
            hmin_row(buf, mat_row(dst, y), src->cols, cn, r);
        }
    }

    fast_free(buf);
}

void erode(struct Mat *dst, const struct Mat *src, int ksize) {
    morph(dst, src, ksize, 0);
}

void dilate(struct Mat *dst, const struct Mat *src, int ksize) {
    morph(dst, src, ksize, 1);
}
