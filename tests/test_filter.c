#include <stdint.h>

#include "filter.h"
#include "test_alloc_util.h"
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

// Full-frame implementations from before the row-streaming refactor. They
// stay here as the byte-exact reference for the ring-buffer versions.
static void old_pad_row(const uchar *s, uchar *pad, int cols, int cn, int r) {
    for (int x = -r; x < cols + r; x++) {
        const uchar *p = s + reflect101(x, cols) * cn;
        for (int c = 0; c < cn; c++) {
            pad[(x + r) * cn + c] = p[c];
        }
    }
}

static void old_box(struct Mat *dst, const struct Mat *src, int ksize) {
    const int rows = src->rows, cols = src->cols;
    const int cn = (int)mat_channel_num(src), n = cols * cn, r = ksize / 2;
    uint16_t *tmp = malloc((size_t)rows * (size_t)n * sizeof(uint16_t));
    uchar    *pad = malloc((size_t)(cols + 2 * r) * (size_t)cn);
    for (int y = 0; y < rows; y++) {
        old_pad_row(mat_row(src, y), pad, cols, cn, r);
        for (int c = 0; c < cn; c++) {
            for (int x = 0; x < cols; x++) {
                unsigned sum = 0;
                for (int i = 0; i < ksize; i++) {
                    sum += pad[(x + i) * cn + c];
                }
                tmp[(size_t)y * (size_t)n + (size_t)(x * cn + c)] =
                    (uint16_t)sum;
            }
        }
    }
    const uint32_t area = (uint32_t)ksize * (uint32_t)ksize;
    for (int y = 0; y < rows; y++) {
        for (int i = 0; i < n; i++) {
            uint32_t sum = 0;
            for (int k = -r; k <= r; k++) {
                sum += tmp[(size_t)reflect101(y + k, rows) * (size_t)n +
                           (size_t)i];
            }
            mat_row(dst, y)[i] = (uchar)((sum + area / 2) / area);
        }
    }
    free(pad);
    free(tmp);
}

static void old_gauss(struct Mat *dst, const struct Mat *src, int ksize) {
    static const int w_tab[4][7] = {{256, 0, 0, 0, 0, 0, 0},
                                    {64, 128, 64, 0, 0, 0, 0},
                                    {16, 64, 96, 64, 16, 0, 0},
                                    {8, 28, 56, 72, 56, 28, 8}};
    const int        rows = src->rows, cols = src->cols;
    const int  cn = (int)mat_channel_num(src), n = cols * cn, r = ksize / 2;
    const int *w   = w_tab[r];
    uint16_t  *tmp = malloc((size_t)rows * (size_t)n * sizeof(uint16_t));
    uchar     *pad = malloc((size_t)(cols + 2 * r) * (size_t)cn);
    for (int y = 0; y < rows; y++) {
        old_pad_row(mat_row(src, y), pad, cols, cn, r);
        for (int i = 0; i < n; i++) {
            unsigned acc = 0;
            for (int k = 0; k < ksize; k++) {
                acc += (unsigned)w[k] * pad[i + k * cn];
            }
            tmp[(size_t)y * (size_t)n + (size_t)i] = (uint16_t)acc;
        }
    }
    for (int y = 0; y < rows; y++) {
        for (int i = 0; i < n; i++) {
            uint32_t acc = 0;
            for (int k = 0; k < ksize; k++) {
                acc += (uint32_t)w[k] *
                       tmp[(size_t)reflect101(y + k - r, rows) * (size_t)n +
                           (size_t)i];
            }
            mat_row(dst, y)[i] = (uchar)((acc + 32768u) >> 16);
        }
    }
    free(pad);
    free(tmp);
}

static void old_sobel(struct Mat *dst, const struct Mat *src, int dx) {
    static const int diff[3] = {-1, 0, 1}, smooth[3] = {1, 2, 1};
    const int       *wh = dx ? diff : smooth, *wv = dx ? smooth : diff;
    const int        rows = src->rows, cols = src->cols;
    int16_t *tmp = malloc((size_t)rows * (size_t)cols * sizeof(int16_t));
    uchar   *pad = malloc((size_t)cols + 2);
    for (int y = 0; y < rows; y++) {
        old_pad_row(mat_row(src, y), pad, cols, 1, 1);
        for (int x = 0; x < cols; x++) {
            tmp[(size_t)y * (size_t)cols + (size_t)x] =
                (int16_t)(wh[0] * pad[x] + wh[1] * pad[x + 1] +
                          wh[2] * pad[x + 2]);
        }
    }
    for (int y = 0; y < rows; y++) {
        const int16_t *t0 =
            tmp + (size_t)reflect101(y - 1, rows) * (size_t)cols;
        const int16_t *t1 = tmp + (size_t)y * (size_t)cols;
        const int16_t *t2 =
            tmp + (size_t)reflect101(y + 1, rows) * (size_t)cols;
        for (int x = 0; x < cols; x++) {
            ((int16_t *)mat_row(dst, y))[x] =
                (int16_t)(wv[0] * t0[x] + wv[1] * t1[x] + wv[2] * t2[x]);
        }
    }
    free(pad);
    free(tmp);
}

static void box_vs_old(int rows, int cols, int ksize, unsigned seed) {
    struct Mat *src = alloc_mat(rows, cols, CV_8UC3);
    struct Mat *dst = alloc_mat(rows, cols, CV_8UC3);
    struct Mat *ref = alloc_mat(rows, cols, CV_8UC3);
    fill_pattern(src, seed);
    box_filter(dst, src, ksize);
    if (cols > 0) {
        old_box(ref, src, ksize);
    }
    CHECK_MAT_BYTES(dst, ref->data);
    free_mat(&src);
    free_mat(&dst);
    free_mat(&ref);
}

static void test_box_stream_small_ksize_sweep(void) {
    for (int ksize = 1; ksize <= 15; ksize += 2) {
        for (int rows = 1; rows <= 2 * ksize + 2; rows++) {
            for (int w = 0; w <= 65; w++) {
                box_vs_old(rows, w, ksize, (unsigned)(rows * 7 + w));
            }
        }
    }
}

static void test_box_stream_every_ksize(void) {
    const int widths[] = {0, 1, 5};
    for (int ksize = 1; ksize <= 255; ksize += 2) {
        const int rows_set[] = {1,     2,         ksize > 1 ? ksize - 1 : 1,
                                ksize, ksize + 1, 2 * ksize + 2};
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 3; j++) {
                box_vs_old(rows_set[i], widths[j], ksize,
                           (unsigned)(ksize + i * 13 + j));
            }
        }
    }
}

static void test_gauss_stream_sweep(void) {
    for (int ksize = 1; ksize <= 7; ksize += 2) {
        for (int rows = 1; rows <= 2 * ksize + 2; rows++) {
            for (int w = 0; w <= 65; w++) {
                struct Mat *src = alloc_mat(rows, w, CV_8UC3);
                struct Mat *dst = alloc_mat(rows, w, CV_8UC3);
                struct Mat *ref = alloc_mat(rows, w, CV_8UC3);
                fill_pattern(src, (unsigned)(rows * 5 + w));
                gaussian_blur(dst, src, ksize);
                if (w > 0) {
                    old_gauss(ref, src, ksize);
                }
                CHECK_MAT_BYTES(dst, ref->data);
                free_mat(&src);
                free_mat(&dst);
                free_mat(&ref);
            }
        }
    }
}

static void test_sobel_stream_sweep(void) {
    for (int dir = 0; dir < 2; dir++) {
        for (int rows = 1; rows <= 8; rows++) {
            for (int w = 0; w <= 65; w++) {
                struct Mat *src = alloc_mat(rows, w, CV_8UC1);
                struct Mat *dst = alloc_mat(rows, w, CV_16SC1);
                struct Mat *ref = alloc_mat(rows, w, CV_16SC1);
                fill_pattern(src, (unsigned)(rows * 3 + w));
                sobel(dst, src, dir == 0, dir == 1);
                if (w > 0) {
                    old_sobel(ref, src, dir == 0);
                }
                CHECK_MAT_BYTES(dst, ref->data);
                free_mat(&src);
                free_mat(&dst);
                free_mat(&ref);
            }
        }
    }
}

// Peak scratch must stay within the documented table plus 64 bytes of
// alignment slack per allocation.
static void check_peak(size_t table, long allocs) {
    CHECK_EQ_INT(alloc_stats.allocs, allocs);
    CHECK_EQ_INT(alloc_stats.allocs, alloc_stats.frees);
    CHECK_EQ_INT(alloc_stats.current, 0);
    CHECK(alloc_stats.peak <= table + 64u * (size_t)allocs);
}

static void test_scratch_peak(void) {
    const int   rows = 40, cols = 64;
    struct Mat *src = alloc_mat(rows, cols, CV_8UC3);
    struct Mat *dst = alloc_mat(rows, cols, CV_8UC3);
    fill_pattern(src, 77);
    const size_t n = (size_t)cols * 3;
    for (int ksize = 3; ksize <= 255; ksize += 2) {
        const size_t slots = (size_t)(ksize < rows ? ksize : rows);
        const size_t table =
            slots * n * 2 + n * 4 + (size_t)(cols + ksize - 1) * 3;
        alloc_counter_start();
        box_filter(dst, src, ksize);
        alloc_counter_stop();
        check_peak(table, 3);
    }
    for (int ksize = 3; ksize <= 7; ksize += 2) {
        const size_t table =
            (size_t)ksize * n * 2 + (size_t)(cols + ksize - 1) * 3;
        alloc_counter_start();
        gaussian_blur(dst, src, ksize);
        alloc_counter_stop();
        check_peak(table, 2);
    }
    free_mat(&src);
    free_mat(&dst);

    src = alloc_mat(rows, cols, CV_8UC1);
    dst = alloc_mat(rows, cols, CV_16SC1);
    fill_pattern(src, 78);
    alloc_counter_start();
    sobel(dst, src, 1, 0);
    alloc_counter_stop();
    check_peak(3 * (size_t)cols * 2 + (size_t)cols + 2, 2);
    free_mat(&src);
    free_mat(&dst);
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
    RUN_TEST(test_box_stream_small_ksize_sweep);
    RUN_TEST(test_box_stream_every_ksize);
    RUN_TEST(test_gauss_stream_sweep);
    RUN_TEST(test_sobel_stream_sweep);
    RUN_TEST(test_scratch_peak);
    return test_finish();
}
