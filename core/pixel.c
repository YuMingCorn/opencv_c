#include "pixel.h"

static void thresh_binary_row(const uchar *restrict s, uchar *restrict d, int n,
                              uchar thresh, uchar maxval) {
    for (int i = 0; i < n; i++) {
        d[i] = s[i] > thresh ? maxval : 0;
    }
}

static void thresh_binary_inv_row(const uchar *restrict s, uchar *restrict d,
                                  int n, uchar thresh, uchar maxval) {
    for (int i = 0; i < n; i++) {
        d[i] = s[i] > thresh ? 0 : maxval;
    }
}

static void thresh_trunc_row(const uchar *restrict s, uchar *restrict d, int n,
                             uchar thresh, uchar maxval) {
    (void)maxval;
    for (int i = 0; i < n; i++) {
        d[i] = s[i] > thresh ? thresh : s[i];
    }
}

static void thresh_tozero_row(const uchar *restrict s, uchar *restrict d, int n,
                              uchar thresh, uchar maxval) {
    (void)maxval;
    for (int i = 0; i < n; i++) {
        d[i] = s[i] > thresh ? s[i] : 0;
    }
}

static void thresh_tozero_inv_row(const uchar *restrict s, uchar *restrict d,
                                  int n, uchar thresh, uchar maxval) {
    (void)maxval;
    for (int i = 0; i < n; i++) {
        d[i] = s[i] > thresh ? 0 : s[i];
    }
}

typedef void (*ThreshRowFn)(const uchar *restrict, uchar *restrict, int, uchar,
                            uchar);

static int is_u8_image_type(size_t t) {
    return t == (size_t)CV_8UC1 || t == (size_t)CV_8UC3 || t == (size_t)CV_8UC4;
}

void threshold(struct Mat *dst, const struct Mat *src, uchar thresh,
               uchar maxval, int type) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(is_u8_image_type(mat_type(src)));
    ASSERT(mat_type(dst) == mat_type(src));
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);

    ThreshRowFn kernel = NULL;
    switch (type) {
    case THRESH_BINARY:
        kernel = thresh_binary_row;
        break;
    case THRESH_BINARY_INV:
        kernel = thresh_binary_inv_row;
        break;
    case THRESH_TRUNC:
        kernel = thresh_trunc_row;
        break;
    case THRESH_TOZERO:
        kernel = thresh_tozero_row;
        break;
    case THRESH_TOZERO_INV:
        kernel = thresh_tozero_inv_row;
        break;
    default:
        ASSERT(0 && "unknown threshold type");
    }

    if (src->rows == 0 || src->cols == 0) {
        return;
    }
    int n = src->cols * (int)mat_channel_num(src);
    for (int y = 0; y < src->rows; y++) {
        kernel(mat_row(src, y), mat_row(dst, y), n, thresh, maxval);
    }
}

static void lut_row(const uchar *restrict s, uchar *restrict d, int n,
                    const uchar *lut) {
    for (int i = 0; i < n; i++) {
        d[i] = lut[s[i]];
    }
}

void mat_lut(struct Mat *dst, const struct Mat *src, const uchar lut[256]) {
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(lut != NULL);
    ASSERT(is_u8_image_type(mat_type(src)));
    ASSERT(mat_type(dst) == mat_type(src));
    ASSERT(dst->rows == src->rows && dst->cols == src->cols);

    if (src->rows == 0 || src->cols == 0) {
        return;
    }
    int n = src->cols * (int)mat_channel_num(src);
    for (int y = 0; y < src->rows; y++) {
        lut_row(mat_row(src, y), mat_row(dst, y), n, lut);
    }
}
