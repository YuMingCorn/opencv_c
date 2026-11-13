#include <stdint.h>

#include "filter.h"
#include "test_mat_util.h"

// Same definition as filter.c, kept separate so the reference is independent.
static int reflect101(int p, int len) {
    if (len == 1) {
        return 0;
    }
    while (p < 0 || p >= len) {
        p = p < 0 ? -p : 2 * (len - 1) - p;
    }
    return p;
}

static void box_bytes(int type, int rows, int cols, const uchar *in, int ksize,
                      const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, type, in);
    struct Mat *dst = alloc_mat(rows, cols, type);
    box_filter(dst, src, ksize);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_box_impulse(void) {
    const uchar in[]  = {0, 0, 0, 0, 90, 0, 0, 0, 0};
    const uchar out[] = {40, 20, 40, 20, 10, 20, 40, 20, 40};
    box_bytes(CV_8UC1, 3, 3, in, 3, out);
}

static void test_box_ramp(void) {
    const uchar in[]  = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const uchar out[] = {4, 4, 4, 5, 5, 5, 6, 6, 6};
    box_bytes(CV_8UC1, 3, 3, in, 3, out);
}

static void test_box_rounding(void) {
    const uchar a[]  = {0, 0, 2};
    const uchar ao[] = {0, 1, 1};
    box_bytes(CV_8UC1, 1, 3, a, 3, ao);
    const uchar b[]  = {0, 0, 1};
    const uchar bo[] = {0, 0, 0};
    box_bytes(CV_8UC1, 1, 3, b, 3, bo);
}

static void test_box_k5_wide_border(void) {
    const uchar in[]  = {0, 0, 0, 0, 100};
    const uchar out[] = {0, 0, 20, 20, 20};
    box_bytes(CV_8UC1, 1, 5, in, 5, out);
}

static void test_box_kernel_larger_than_image(void) {
    const uchar in[]  = {5, 5, 5, 5};
    const uchar out[] = {5, 5, 5, 5};
    box_bytes(CV_8UC1, 2, 2, in, 7, out);
}

static void test_box_rgb(void) {
    const uchar in[]  = {10, 5, 40, 20, 5, 30, 30, 5, 20, 40, 5, 10};
    const uchar out[] = {17, 5, 33, 20, 5, 30, 30, 5, 20, 33, 5, 17};
    box_bytes(CV_8UC3, 1, 4, in, 3, out);
}

static void test_box_ksize1_identity(void) {
    struct Mat *src = alloc_mat(3, 7, CV_8UC3);
    struct Mat *dst = alloc_mat(3, 7, CV_8UC3);
    fill_pattern(src, 11);
    box_filter(dst, src, 1);
    CHECK_MAT_BYTES(dst, src->data);
    free_mat(&src);
    free_mat(&dst);
}

static void box_reference(struct Mat *dst, const struct Mat *src, int ksize) {
    int cn = (int)mat_channel_num(src);
    int r  = ksize / 2;
    for (int y = 0; y < src->rows; y++) {
        for (int x = 0; x < src->cols; x++) {
            for (int c = 0; c < cn; c++) {
                unsigned sum = 0;
                for (int i = -r; i <= r; i++) {
                    for (int j = -r; j <= r; j++) {
                        int yy = reflect101(y + i, src->rows);
                        int xx = reflect101(x + j, src->cols);
                        sum += mat_row(src, yy)[xx * cn + c];
                    }
                }
                unsigned area               = (unsigned)(ksize * ksize);
                mat_row(dst, y)[x * cn + c] = (uchar)((sum + area / 2) / area);
            }
        }
    }
}

static void test_box_width_sweep(void) {
    const int ks[] = {3, 5};
    for (int k = 0; k < 2; k++) {
        for (int w = 0; w <= 65; w++) {
            struct Mat *src = alloc_mat(3, w, CV_8UC3);
            struct Mat *dst = alloc_mat(3, w, CV_8UC3);
            struct Mat *ref = alloc_mat(3, w, CV_8UC3);
            fill_pattern(src, (unsigned)w);
            box_filter(dst, src, ks[k]);
            box_reference(ref, src, ks[k]);
            CHECK_MAT_BYTES(dst, ref->data);
            free_mat(&src);
            free_mat(&dst);
            free_mat(&ref);
        }
    }
}

static void gauss_bytes(int type, int rows, int cols, const uchar *in,
                        int ksize, const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, type, in);
    struct Mat *dst = alloc_mat(rows, cols, type);
    gaussian_blur(dst, src, ksize);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_gauss_impulse_k3(void) {
    uchar in[25]             = {0};
    in[2 * 5 + 2]            = 255;
    uchar       out[25]      = {0};
    const uchar centre[3][3] = {{16, 32, 16}, {32, 64, 32}, {16, 32, 16}};
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            out[(y + 1) * 5 + (x + 1)] = centre[y][x];
        }
    }
    gauss_bytes(CV_8UC1, 5, 5, in, 3, out);
}

static void test_gauss_impulse_k5(void) {
    uchar in[25]      = {0};
    in[2 * 5 + 2]     = 255;
    const uchar out[] = {4,  8,  12, 8,  4,  8,  16, 24, 16, 8,  12, 24, 36,
                         24, 12, 8,  16, 24, 16, 8,  4,  8,  12, 8,  4};
    gauss_bytes(CV_8UC1, 5, 5, in, 5, out);
}

static void test_gauss_row_k3(void) {
    const uchar in[]  = {10, 20, 30, 40};
    const uchar out[] = {15, 20, 30, 35};
    gauss_bytes(CV_8UC1, 1, 4, in, 3, out);
}

static void test_gauss_row_k5_impulse(void) {
    const uchar in[]  = {0, 0, 255, 0, 0, 0};
    const uchar out[] = {32, 64, 96, 64, 16, 0};
    gauss_bytes(CV_8UC1, 1, 6, in, 5, out);
}

static void test_gauss_2x3_k3(void) {
    const uchar in[]  = {0, 100, 200, 50, 150, 250};
    const uchar out[] = {75, 125, 175, 75, 125, 175};
    gauss_bytes(CV_8UC1, 2, 3, in, 3, out);
}

static void test_gauss_constant_k7(void) {
    const uchar in[]  = {7, 7, 7, 7};
    const uchar out[] = {7, 7, 7, 7};
    gauss_bytes(CV_8UC1, 2, 2, in, 7, out);
}

static void test_gauss_rgb(void) {
    const uchar in[]  = {10, 128, 40, 20, 128, 30, 30, 128, 20, 40, 128, 10};
    const uchar out[] = {15, 128, 35, 20, 128, 30, 30, 128, 20, 35, 128, 15};
    gauss_bytes(CV_8UC3, 1, 4, in, 3, out);
}

static const int gauss_ref_w[4][7] = {
    {0, 0, 0, 256, 0, 0, 0},
    {0, 0, 64, 128, 64, 0, 0},
    {0, 16, 64, 96, 64, 16, 0},
    {8, 28, 56, 72, 56, 28, 8},
};

static void gauss_reference(struct Mat *dst, const struct Mat *src, int ksize) {
    const int *w  = gauss_ref_w[ksize / 2];
    int        cn = (int)mat_channel_num(src);
    for (int y = 0; y < src->rows; y++) {
        for (int x = 0; x < src->cols; x++) {
            for (int c = 0; c < cn; c++) {
                uint32_t acc = 0;
                for (int i = -3; i <= 3; i++) {
                    for (int j = -3; j <= 3; j++) {
                        int yy = reflect101(y + i, src->rows);
                        int xx = reflect101(x + j, src->cols);
                        acc += (uint32_t)(w[i + 3] * w[j + 3]) *
                               mat_row(src, yy)[xx * cn + c];
                    }
                }
                mat_row(dst, y)[x * cn + c] = (uchar)((acc + 32768u) >> 16);
            }
        }
    }
}

static void test_gauss_width_sweep(void) {
    for (int ksize = 3; ksize <= 7; ksize += 2) {
        for (int w = 0; w <= 65; w++) {
            struct Mat *src = alloc_mat(3, w, CV_8UC3);
            struct Mat *dst = alloc_mat(3, w, CV_8UC3);
            struct Mat *ref = alloc_mat(3, w, CV_8UC3);
            fill_pattern(src, (unsigned)w + 5u);
            gaussian_blur(dst, src, ksize);
            gauss_reference(ref, src, ksize);
            CHECK_MAT_BYTES(dst, ref->data);
            free_mat(&src);
            free_mat(&dst);
            free_mat(&ref);
        }
    }
}

static void sobel_i16(int rows, int cols, const uchar *in, int dx, int dy,
                      const int16_t *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, CV_8UC1, in);
    struct Mat *dst = alloc_mat(rows, cols, CV_16SC1);
    sobel(dst, src, dx, dy);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_sobel_vertical_edge(void) {
    const uchar   in[]       = {0, 0, 100, 100, 0, 0, 100, 100, 0, 0, 100, 100};
    const int16_t dx_out[]   = {0, 400, 400, 0, 0, 400, 400, 0, 0, 400, 400, 0};
    const int16_t dy_out[12] = {0};
    sobel_i16(3, 4, in, 1, 0, dx_out);
    sobel_i16(3, 4, in, 0, 1, dy_out);
}

static void test_sobel_negative(void) {
    const uchar   in[]  = {100, 100, 0, 0, 100, 100, 0, 0};
    const int16_t out[] = {0, -400, -400, 0, 0, -400, -400, 0};
    sobel_i16(2, 4, in, 1, 0, out);
}

static void test_sobel_ramp(void) {
    const uchar   in[]     = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const int16_t dx_out[] = {0, 8, 0, 0, 8, 0, 0, 8, 0};
    const int16_t dy_out[] = {0, 0, 0, 24, 24, 24, 0, 0, 0};
    sobel_i16(3, 3, in, 1, 0, dx_out);
    sobel_i16(3, 3, in, 0, 1, dy_out);
}

static void test_sobel_max_range(void) {
    const uchar   in[]     = {0, 0, 0, 0, 0, 0, 255, 255, 255};
    const int16_t dy_out[] = {0, 0, 0, 1020, 1020, 1020, 0, 0, 0};
    sobel_i16(3, 3, in, 0, 1, dy_out);
    const uchar   row[]    = {0, 0, 255};
    const int16_t dx_out[] = {0, 1020, 0};
    sobel_i16(1, 3, row, 1, 0, dx_out);
}

static void test_sobel_two_wide(void) {
    const uchar   in[]  = {0, 255};
    const int16_t out[] = {0, 0};
    sobel_i16(1, 2, in, 1, 0, out);
}

static void sobel_reference(struct Mat *dst, const struct Mat *src, int dx,
                            int dy) {
    static const int kx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    static const int ky[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    const int (*k)[3]         = dx ? kx : ky;
    (void)dy;
    for (int y = 0; y < src->rows; y++) {
        for (int x = 0; x < src->cols; x++) {
            int acc = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    int yy = reflect101(y + i, src->rows);
                    int xx = reflect101(x + j, src->cols);
                    acc += k[i + 1][j + 1] * mat_row(src, yy)[xx];
                }
            }
            ((int16_t *)mat_row(dst, y))[x] = (int16_t)acc;
        }
    }
}

static void test_sobel_width_sweep(void) {
    for (int dir = 0; dir < 2; dir++) {
        for (int w = 0; w <= 65; w++) {
            struct Mat *src = alloc_mat(3, w, CV_8UC1);
            struct Mat *dst = alloc_mat(3, w, CV_16SC1);
            struct Mat *ref = alloc_mat(3, w, CV_16SC1);
            fill_pattern(src, (unsigned)w + 9u);
            sobel(dst, src, dir == 0, dir == 1);
            sobel_reference(ref, src, dir == 0, dir == 1);
            CHECK_MAT_BYTES(dst, ref->data);
            free_mat(&src);
            free_mat(&dst);
            free_mat(&ref);
        }
    }
}

int main(void) {
    RUN_TEST(test_box_impulse);
    RUN_TEST(test_box_ramp);
    RUN_TEST(test_box_rounding);
    RUN_TEST(test_box_k5_wide_border);
    RUN_TEST(test_box_kernel_larger_than_image);
    RUN_TEST(test_box_rgb);
    RUN_TEST(test_box_ksize1_identity);
    RUN_TEST(test_box_width_sweep);
    RUN_TEST(test_gauss_impulse_k3);
    RUN_TEST(test_gauss_impulse_k5);
    RUN_TEST(test_gauss_row_k3);
    RUN_TEST(test_gauss_row_k5_impulse);
    RUN_TEST(test_gauss_2x3_k3);
    RUN_TEST(test_gauss_constant_k7);
    RUN_TEST(test_gauss_rgb);
    RUN_TEST(test_gauss_width_sweep);
    RUN_TEST(test_sobel_vertical_edge);
    RUN_TEST(test_sobel_negative);
    RUN_TEST(test_sobel_ramp);
    RUN_TEST(test_sobel_max_range);
    RUN_TEST(test_sobel_two_wide);
    RUN_TEST(test_sobel_width_sweep);
    return test_finish();
}
