#include "filter.h"

#include <stdint.h>

#include "utils.h"

static inline int border_reflect_101(int p, int len) {
    if (len == 1) {
        return 0;
    }
    while (p < 0 || p >= len) {
        p = p < 0 ? -p : 2 * (len - 1) - p;
    }
    return p;
}

static int is_u8_image_type(const struct Mat *m) {
    size_t t = mat_type(m);
    return t == (size_t)CV_8UC1 || t == (size_t)CV_8UC3 || t == (size_t)CV_8UC4;
}

// Copies one row into pad, extending r pixels on each side with reflect 101.
// pad holds (cols + 2r) * cn bytes.
static void pad_row(const uchar *restrict s, uchar *restrict pad, int cols,
                    int cn, int r) {
    for (int x = -r; x < cols + r; x++) {
        const uchar *p = s + border_reflect_101(x, cols) * cn;
        uchar       *q = pad + (x + r) * cn;
        for (int c = 0; c < cn; c++) {
            q[c] = p[c];
        }
    }
}

// The horizontal pass keeps a ring of nslots = min(window, rows) rows, and
// row y lives in slot y % nslots. Output row y reads only rows in
// [max(0, y - r), min(rows - 1, y + r)], at most `window` consecutive rows,
// so a slot is never reused while its row is still needed.
static inline size_t ring_slot(int row, int nslots, size_t n) {
    return (size_t)(row % nslots) * n;
}

// Box horizontal pass: sliding window sum of ksize pixels per channel.
// pad holds (width + ksize - 1) * cn bytes, out holds width * cn.
static void box_hrow(const uchar *restrict pad, uint16_t *restrict out,
                     int width, int cn, int ksize) {
    for (int c = 0; c < cn; c++) {
        unsigned sum = 0;
        for (int i = 0; i < ksize; i++) {
            sum += pad[i * cn + c];
        }
        out[c] = (uint16_t)sum;
        for (int x = 1; x < width; x++) {
            sum += pad[(x + ksize - 1) * cn + c];
            sum -= pad[(x - 1) * cn + c];
            out[x * cn + c] = (uint16_t)sum;
        }
    }
}

static void colsum_add(uint32_t *restrict sum, const uint16_t *restrict t,
                       int n) {
    for (int i = 0; i < n; i++) {
        sum[i] += t[i];
    }
}

static void colsum_sub(uint32_t *restrict sum, const uint16_t *restrict t,
                       int n) {
    for (int i = 0; i < n; i++) {
        sum[i] -= t[i];
    }
}

static void box_vrow(const uint32_t *restrict sum, uchar *restrict d, int n,
                     uint32_t area) {
    for (int i = 0; i < n; i++) {
        d[i] = (uchar)((sum[i] + area / 2) / area);
    }
}

void box_filter(struct Mat *dst, const struct Mat *src, int ksize) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(is_u8_image_type(src));
    ASSERT(mat_type(dst) == mat_type(src));
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);
    ASSERT(ksize >= 1 && ksize <= 255 && (ksize & 1));
    if (src->rows == 0 || src->cols == 0) {
        return;
    }

    const int    rows   = src->rows;
    const int    cn     = (int)mat_channel_num(src);
    const int    n      = src->cols * cn;
    const int    r      = ksize / 2;
    const int    nslots = ksize < rows ? ksize : rows;
    const size_t un     = (size_t)n;

    uint16_t *ring = fast_malloc((size_t)nslots * un * sizeof(uint16_t));
    uchar    *pad  = fast_malloc((size_t)(src->cols + 2 * r) * (size_t)cn);
    uint32_t *sum  = fast_malloc(un * sizeof(uint32_t));
    ASSERT(ring != NULL && pad != NULL && sum != NULL);

    // `next` is the first row whose horizontal pass is not computed yet.
    int next = 0;
    for (; next <= (r < rows - 1 ? r : rows - 1); next++) {
        pad_row(mat_row(src, next), pad, src->cols, cn, r);
        box_hrow(pad, ring + ring_slot(next, nslots, un), src->cols, cn, ksize);
    }
    for (int i = 0; i < n; i++) {
        sum[i] = 0;
    }
    for (int i = -r; i <= r; i++) {
        colsum_add(
            sum, ring + ring_slot(border_reflect_101(i, rows), nslots, un), n);
    }
    const uint32_t area = (uint32_t)ksize * (uint32_t)ksize;
    for (int y = 0; y < rows; y++) {
        box_vrow(sum, mat_row(dst, y), n, area);
        // Subtract the outgoing row before its slot can be overwritten.
        colsum_sub(
            sum, ring + ring_slot(border_reflect_101(y - r, rows), nslots, un),
            n);
        const int in  = y + r + 1;
        const int top = in < rows - 1 ? in : rows - 1;
        for (; next <= top; next++) {
            pad_row(mat_row(src, next), pad, src->cols, cn, r);
            box_hrow(pad, ring + ring_slot(next, nslots, un), src->cols, cn,
                     ksize);
        }
        colsum_add(
            sum, ring + ring_slot(border_reflect_101(in, rows), nslots, un), n);
    }

    fast_free(sum);
    fast_free(pad);
    fast_free(ring);
}

static const int gauss_weights[4][7] = {
    {256, 0, 0, 0, 0, 0, 0},
    {64, 128, 64, 0, 0, 0, 0},
    {16, 64, 96, 64, 16, 0, 0},
    {8, 28, 56, 72, 56, 28, 8},
};

// Weighted horizontal pass. Largest sum is 256 * 255, which fits uint16_t.
static void gauss_hrow(const uchar *restrict pad, uint16_t *restrict out,
                       int width, int cn, const int *w, int ksize) {
    int n = width * cn;
    for (int i = 0; i < n; i++) {
        unsigned acc = 0;
        for (int k = 0; k < ksize; k++) {
            acc += (unsigned)w[k] * pad[i + k * cn];
        }
        out[i] = (uint16_t)acc;
    }
}

static void gauss_vrow(const uint16_t *const *restrict rows_in,
                       uchar *restrict d, int n, const int *w, int ksize) {
    for (int i = 0; i < n; i++) {
        uint32_t acc = 0;
        for (int k = 0; k < ksize; k++) {
            acc += (uint32_t)w[k] * rows_in[k][i];
        }
        d[i] = (uchar)((acc + 32768u) >> 16);
    }
}

void gaussian_blur(struct Mat *dst, const struct Mat *src, int ksize) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(is_u8_image_type(src));
    ASSERT(mat_type(dst) == mat_type(src));
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);
    ASSERT(ksize == 1 || ksize == 3 || ksize == 5 || ksize == 7);
    if (src->rows == 0 || src->cols == 0) {
        return;
    }

    const int    rows   = src->rows;
    const int    cn     = (int)mat_channel_num(src);
    const int    n      = src->cols * cn;
    const int    r      = ksize / 2;
    const int   *w      = gauss_weights[r];
    const int    nslots = ksize < rows ? ksize : rows;
    const size_t un     = (size_t)n;

    uint16_t *ring = fast_malloc((size_t)nslots * un * sizeof(uint16_t));
    uchar    *pad  = fast_malloc((size_t)(src->cols + 2 * r) * (size_t)cn);
    ASSERT(ring != NULL && pad != NULL);

    int next = 0;
    for (int y = 0; y < rows; y++) {
        const int top = y + r < rows - 1 ? y + r : rows - 1;
        for (; next <= top; next++) {
            pad_row(mat_row(src, next), pad, src->cols, cn, r);
            gauss_hrow(pad, ring + ring_slot(next, nslots, un), src->cols, cn,
                       w, ksize);
        }
        const uint16_t *in[7];
        for (int k = 0; k < ksize; k++) {
            in[k] = ring +
                    ring_slot(border_reflect_101(y + k - r, rows), nslots, un);
        }
        gauss_vrow(in, mat_row(dst, y), n, w, ksize);
    }

    fast_free(pad);
    fast_free(ring);
}

// Sobel horizontal pass with a 3-tap kernel; |result| <= 1020 fits int16_t.
static void sobel_hrow(const uchar *restrict pad, int16_t *restrict out, int n,
                       int cn, const int *w) {
    for (int i = 0; i < n; i++) {
        out[i] = (int16_t)(w[0] * pad[i] + w[1] * pad[i + cn] +
                           w[2] * pad[i + 2 * cn]);
    }
}

static void sobel_vrow(const int16_t *restrict t0, const int16_t *restrict t1,
                       const int16_t *restrict t2, int16_t *restrict d, int n,
                       const int *w) {
    for (int i = 0; i < n; i++) {
        d[i] = (int16_t)(w[0] * t0[i] + w[1] * t1[i] + w[2] * t2[i]);
    }
}

void sobel(struct Mat *dst, const struct Mat *src, int dx, int dy) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(mat_type(src) == (size_t)CV_8UC1);
    ASSERT(mat_type(dst) == (size_t)CV_16SC1);
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);
    ASSERT((dx == 1 && dy == 0) || (dx == 0 && dy == 1));
    if (src->rows == 0 || src->cols == 0) {
        return;
    }

    static const int diff[3]   = {-1, 0, 1};
    static const int smooth[3] = {1, 2, 1};
    const int       *wh        = dx ? diff : smooth;
    const int       *wv        = dx ? smooth : diff;

    const int    rows   = src->rows;
    const int    cols   = src->cols;
    const int    nslots = 3 < rows ? 3 : rows;
    const size_t uc     = (size_t)cols;

    int16_t *ring = fast_malloc((size_t)nslots * uc * sizeof(int16_t));
    uchar   *pad  = fast_malloc((size_t)cols + 2);
    ASSERT(ring != NULL && pad != NULL);

    int next = 0;
    for (int y = 0; y < rows; y++) {
        const int top = y + 1 < rows - 1 ? y + 1 : rows - 1;
        for (; next <= top; next++) {
            pad_row(mat_row(src, next), pad, cols, 1, 1);
            sobel_hrow(pad, ring + ring_slot(next, nslots, uc), cols, 1, wh);
        }
        const int16_t *t0 =
            ring + ring_slot(border_reflect_101(y - 1, rows), nslots, uc);
        const int16_t *t1 = ring + ring_slot(y, nslots, uc);
        const int16_t *t2 =
            ring + ring_slot(border_reflect_101(y + 1, rows), nslots, uc);
        sobel_vrow(t0, t1, t2, (int16_t *)mat_row(dst, y), cols, wv);
    }

    fast_free(pad);
    fast_free(ring);
}
