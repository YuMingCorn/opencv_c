#include "convert_f32.h"

#if IMPROC_HAS_FLOAT

static int channels_ok(int cn) { return cn == 1 || cn == 3 || cn == 4; }

// Saturating round-half-up float -> uint8. No libm.
static inline uchar float_to_u8(float t) {
    if (!(t > 0.0f)) {
        return 0; // also NaN and -inf
    }
    if (t >= 255.0f) {
        return 255;
    }
    int32_t i = (int32_t)t;
    if (t - (float)i >= 0.5f) {
        i++;
    }
    return (uchar)i;
}

static void row_u8_to_f32(const uchar *restrict s, float *restrict d, int n,
                          float alpha, float beta) {
    for (int i = 0; i < n; i++) {
        d[i] = (float)s[i] * alpha + beta;
    }
}

static void row_f32_to_u8(const float *restrict s, uchar *restrict d, int n,
                          float alpha, float beta) {
    for (int i = 0; i < n; i++) {
        d[i] = float_to_u8(s[i] * alpha + beta);
    }
}

static void row_f32_to_f32(const float *restrict s, float *restrict d, int n,
                           float alpha, float beta) {
    for (int i = 0; i < n; i++) {
        d[i] = s[i] * alpha + beta;
    }
}

void convert_to(struct Mat *dst, const struct Mat *src, float alpha,
                float beta) {
    ASSERT(dst != src && dst->data != src->data);
    size_t sd = mat_depth(src);
    size_t dd = mat_depth(dst);
    int    cn = (int)mat_channel_num(src);
    ASSERT(channels_ok(cn) && (int)mat_channel_num(dst) == cn);
    ASSERT((sd == CV_8U && dd == CV_32F) || (sd == CV_32F && dd == CV_8U) ||
           (sd == CV_32F && dd == CV_32F));
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);

    if (src->rows == 0 || src->cols == 0) {
        return;
    }
    int n = src->cols * cn; // Mat construction guarantees cols * cn <= INT_MAX
    for (int y = 0; y < src->rows; y++) {
        const uchar *s = mat_row(src, y);
        uchar       *d = mat_row(dst, y);
        if (sd == CV_8U) {
            row_u8_to_f32(s, (float *)d, n, alpha, beta);
        } else if (dd == CV_8U) {
            row_f32_to_u8((const float *)s, d, n, alpha, beta);
        } else {
            row_f32_to_f32((const float *)s, (float *)d, n, alpha, beta);
        }
    }
}

#endif // IMPROC_HAS_FLOAT
