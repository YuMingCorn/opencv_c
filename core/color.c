#include "color.h"

static inline uchar luma(unsigned r, unsigned g, unsigned b) {
    return (uchar)((77u * r + 150u * g + 29u * b + 128u) >> 8);
}

static void rgb2gray_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3) {
        d[x] = luma(s[0], s[1], s[2]);
    }
}

static void bgr2gray_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3) {
        d[x] = luma(s[2], s[1], s[0]);
    }
}

static void rgba2gray_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, s += 4) {
        d[x] = luma(s[0], s[1], s[2]);
    }
}

static void bgra2gray_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, s += 4) {
        d[x] = luma(s[2], s[1], s[0]);
    }
}

static void gray2rgb_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, d += 3) {
        d[0] = d[1] = d[2] = s[x];
    }
}

static void gray2rgba_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, d += 4) {
        d[0] = d[1] = d[2] = s[x];
        d[3]               = 255;
    }
}

static void rgb2bgr_row(const uchar *restrict s, uchar *restrict d, int width) {
    for (int x = 0; x < width; x++, s += 3, d += 3) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
    }
}

static void rgba2bgra_row(const uchar *restrict s, uchar *restrict d,
                          int width) {
    for (int x = 0; x < width; x++, s += 4, d += 4) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
        d[3] = s[3];
    }
}

static void rgb2rgba_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3, d += 4) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = 255;
    }
}

static void rgba2rgb_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 4, d += 3) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
    }
}

static void rgb2bgra_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 3, d += 4) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
        d[3] = 255;
    }
}

static void rgba2bgr_row(const uchar *restrict s, uchar *restrict d,
                         int width) {
    for (int x = 0; x < width; x++, s += 4, d += 3) {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
    }
}

typedef void (*color_row_fn)(const uchar *restrict, uchar *restrict, int);

struct ColorConversion {
    int          code;
    int          scn;
    int          dcn;
    color_row_fn kernel;
};

static const struct ColorConversion conversions[] = {
    {COLOR_RGB2GRAY, 3, 1, rgb2gray_row},
    {COLOR_BGR2GRAY, 3, 1, bgr2gray_row},
    {COLOR_RGBA2GRAY, 4, 1, rgba2gray_row},
    {COLOR_BGRA2GRAY, 4, 1, bgra2gray_row},
    {COLOR_GRAY2RGB, 1, 3, gray2rgb_row},
    {COLOR_GRAY2RGBA, 1, 4, gray2rgba_row},
    {COLOR_RGB2BGR, 3, 3, rgb2bgr_row},
    {COLOR_RGBA2BGRA, 4, 4, rgba2bgra_row},
    {COLOR_RGB2RGBA, 3, 4, rgb2rgba_row},
    {COLOR_RGBA2RGB, 4, 3, rgba2rgb_row},
    {COLOR_RGB2BGRA, 3, 4, rgb2bgra_row},
    {COLOR_RGBA2BGR, 4, 3, rgba2bgr_row},
};

void cvt_color(struct Mat *dst, const struct Mat *src, int code) {
    const struct ColorConversion *conv = NULL;
    for (size_t i = 0; i < sizeof(conversions) / sizeof(conversions[0]); i++) {
        if (conversions[i].code == code) {
            conv = &conversions[i];
            break;
        }
    }
    ASSERT(conv != NULL);
    ASSERT(dst != src && dst->data != src->data);
    ASSERT(mat_type(src) == (size_t)CV_MAKETYPE(CV_8U, conv->scn));
    ASSERT(mat_type(dst) == (size_t)CV_MAKETYPE(CV_8U, conv->dcn));
    ASSERT(src->rows == dst->rows && src->cols == dst->cols);
    if (src->rows == 0 || src->cols == 0) {
        return;
    }
    for (int y = 0; y < src->rows; y++) {
        conv->kernel(mat_row(src, y), mat_row(dst, y), src->cols);
    }
}
