#include <stdint.h>

#include "filter.h"
#include "pixel.h"
#include "test_mat_util.h"

static void run_threshold(int type, int rows, int cols, int mat_t,
                          const uchar *in, uchar thresh, uchar maxval,
                          const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, mat_t, in);
    struct Mat *dst = alloc_mat(rows, cols, mat_t);
    threshold(dst, src, thresh, maxval, type);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_threshold_types_c3(void) {
    const uchar in[]      = {0, 99, 100, 101, 255, 50};
    const uchar bin[]     = {0, 0, 0, 200, 200, 0};
    const uchar bin_inv[] = {200, 200, 200, 0, 0, 200};
    const uchar trunc[]   = {0, 99, 100, 100, 100, 50};
    const uchar tozero[]  = {0, 0, 0, 101, 255, 0};
    const uchar tozinv[]  = {0, 99, 100, 0, 0, 50};
    run_threshold(THRESH_BINARY, 1, 2, CV_8UC3, in, 100, 200, bin);
    run_threshold(THRESH_BINARY_INV, 1, 2, CV_8UC3, in, 100, 200, bin_inv);
    run_threshold(THRESH_TRUNC, 1, 2, CV_8UC3, in, 100, 200, trunc);
    run_threshold(THRESH_TOZERO, 1, 2, CV_8UC3, in, 100, 200, tozero);
    run_threshold(THRESH_TOZERO_INV, 1, 2, CV_8UC3, in, 100, 200, tozinv);
}

static void test_threshold_types_c1(void) {
    const uchar in[]      = {0, 99, 100, 101, 255, 50};
    const uchar bin[]     = {0, 0, 0, 200, 200, 0};
    const uchar bin_inv[] = {200, 200, 200, 0, 0, 200};
    const uchar trunc[]   = {0, 99, 100, 100, 100, 50};
    const uchar tozero[]  = {0, 0, 0, 101, 255, 0};
    const uchar tozinv[]  = {0, 99, 100, 0, 0, 50};
    run_threshold(THRESH_BINARY, 2, 3, CV_8UC1, in, 100, 200, bin);
    run_threshold(THRESH_BINARY_INV, 2, 3, CV_8UC1, in, 100, 200, bin_inv);
    run_threshold(THRESH_TRUNC, 2, 3, CV_8UC1, in, 100, 200, trunc);
    run_threshold(THRESH_TOZERO, 2, 3, CV_8UC1, in, 100, 200, tozero);
    run_threshold(THRESH_TOZERO_INV, 2, 3, CV_8UC1, in, 100, 200, tozinv);
}

static void test_threshold_c4_alpha_normal(void) {
    const uchar in[]  = {50, 150, 250, 255};
    const uchar out[] = {0, 255, 255, 255};
    run_threshold(THRESH_BINARY, 1, 1, CV_8UC4, in, 100, 255, out);
}

static void test_threshold_extremes(void) {
    const uchar in[]   = {0, 1, 128, 254, 255};
    const uchar none[] = {0, 0, 0, 0, 0};
    const uchar zero[] = {0, 9, 9, 9, 9};
    run_threshold(THRESH_BINARY, 1, 5, CV_8UC1, in, 255, 9, none);
    run_threshold(THRESH_BINARY, 1, 5, CV_8UC1, in, 0, 9, zero);
}

static uchar ref_threshold(int type, uchar s, uchar t, uchar m) {
    int gt = s > t;
    switch (type) {
    case THRESH_BINARY:
        return gt ? m : 0;
    case THRESH_BINARY_INV:
        return gt ? 0 : m;
    case THRESH_TRUNC:
        return gt ? t : s;
    case THRESH_TOZERO:
        return gt ? s : 0;
    default:
        return gt ? 0 : s;
    }
}

static void test_threshold_width_sweep(void) {
    const int types[] = {THRESH_BINARY, THRESH_TRUNC};
    for (int ti = 0; ti < 2; ti++) {
        for (int w = 0; w <= 65; w++) {
            struct Mat *src = alloc_mat(3, w, CV_8UC3);
            struct Mat *dst = alloc_mat(3, w, CV_8UC3);
            fill_pattern(src, (unsigned)w);
            threshold(dst, src, 120, 77, types[ti]);
            for (int y = 0; y < 3; y++) {
                for (int x = 0; x < w; x++) {
                    const uchar *p = mat_at(src, y, x);
                    const uchar *q = mat_at(dst, y, x);
                    for (int c = 0; c < 3; c++) {
                        CHECK_EQ_INT(q[c],
                                     ref_threshold(types[ti], p[c], 120, 77));
                    }
                }
            }
            free_mat(&src);
            free_mat(&dst);
        }
    }
}

static void test_lut_invert(void) {
    const uchar in[]  = {0, 1, 128, 255, 10, 20};
    const uchar out[] = {255, 254, 127, 0, 245, 235};
    uchar       lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = (uchar)(255 - i);
    struct Mat *src = mat_from_bytes(1, 2, CV_8UC3, in);
    struct Mat *dst = alloc_mat(1, 2, CV_8UC3);
    mat_lut(dst, src, lut);
    CHECK_MAT_BYTES(dst, out);
    free_mat(&src);
    free_mat(&dst);
}

static void test_lut_identity(void) {
    const int types[] = {CV_8UC1, CV_8UC3, CV_8UC4};
    uchar     lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = (uchar)i;
    for (int t = 0; t < 3; t++) {
        struct Mat *src = alloc_mat(5, 7, types[t]);
        struct Mat *dst = alloc_mat(5, 7, types[t]);
        fill_pattern(src, 5);
        mat_lut(dst, src, lut);
        CHECK_MAT_BYTES(dst, src->data);
        free_mat(&src);
        free_mat(&dst);
    }
}

static void test_lut_constant(void) {
    uchar lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = 7;
    struct Mat *src = alloc_mat(4, 6, CV_8UC3);
    struct Mat *dst = alloc_mat(4, 6, CV_8UC3);
    fill_pattern(src, 1);
    mat_lut(dst, src, lut);
    for (size_t i = 0; i < (size_t)dst->rows * dst->step[0]; i++) {
        CHECK_EQ_INT(dst->data[i], 7);
    }
    free_mat(&src);
    free_mat(&dst);
}

static void test_lut_width_sweep(void) {
    uchar lut[256];
    for (int i = 0; i < 256; i++)
        lut[i] = (uchar)(i * 3u + 11u);
    for (int w = 0; w <= 65; w++) {
        struct Mat *src = alloc_mat(3, w, CV_8UC3);
        struct Mat *dst = alloc_mat(3, w, CV_8UC3);
        fill_pattern(src, (unsigned)w);
        mat_lut(dst, src, lut);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                const uchar *p = mat_at(src, y, x);
                const uchar *q = mat_at(dst, y, x);
                for (int c = 0; c < 3; c++)
                    CHECK_EQ_INT(q[c], lut[p[c]]);
            }
        }
        free_mat(&src);
        free_mat(&dst);
    }
}

static void scale_c1(int alpha, int beta, int n, const uchar *in,
                     const uchar *expected) {
    struct Mat *src = mat_from_bytes(1, n, CV_8UC1, in);
    struct Mat *dst = alloc_mat(1, n, CV_8UC1);
    convert_scale(dst, src, alpha, beta);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_convert_scale_c3(void) {
    const uchar in[]  = {0, 1, 100, 127, 128, 255};
    const uchar out[] = {0, 2, 200, 254, 255, 255};
    struct Mat *src   = mat_from_bytes(1, 2, CV_8UC3, in);
    struct Mat *dst   = alloc_mat(1, 2, CV_8UC3);
    convert_scale(dst, src, 512, 0);
    CHECK_MAT_BYTES(dst, out);
    free_mat(&src);
    free_mat(&dst);
}

static void test_convert_scale_values(void) {
    const uchar a[]   = {0, 1, 2, 3, 255};
    const uchar a_o[] = {10, 11, 11, 12, 138};
    scale_c1(128, 10, 5, a, a_o);
    const uchar b[]   = {0, 1, 255};
    const uchar b_o[] = {255, 254, 0};
    scale_c1(-256, 255, 3, b, b_o);
    const uchar c[]   = {0, 99, 100, 101, 255};
    const uchar c_o[] = {0, 0, 0, 1, 155};
    scale_c1(256, -100, 5, c, c_o);
    const uchar d[]   = {0, 1, 2, 3, 170, 171};
    const uchar d_o[] = {0, 2, 3, 5, 255, 255};
    scale_c1(384, 0, 6, d, d_o);
}

static void test_convert_scale_identity(void) {
    struct Mat *src = alloc_mat(4, 9, CV_8UC4);
    struct Mat *dst = alloc_mat(4, 9, CV_8UC4);
    fill_pattern(src, 2);
    convert_scale(dst, src, 256, 0);
    CHECK_MAT_BYTES(dst, src->data);
    free_mat(&src);
    free_mat(&dst);
}

static void abs_c1(int alpha, int beta, int n, const int16_t *in,
                   const uchar *expected) {
    struct Mat *src = mat_from_bytes(1, n, CV_16SC1, in);
    struct Mat *dst = alloc_mat(1, n, CV_8UC1);
    convert_scale_abs(dst, src, alpha, beta);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_convert_scale_abs_values(void) {
    const int16_t a[]   = {-300, -255, -1, 0, 1, 255, 300, -32768};
    const uchar   a_o[] = {255, 255, 1, 0, 1, 255, 255, 255};
    abs_c1(256, 0, 8, a, a_o);
    const int16_t b[]   = {-3, -1, 1, 3, 510, -510};
    const uchar   b_o[] = {2, 1, 1, 2, 255, 255};
    abs_c1(128, 0, 6, b, b_o);
    const int16_t c[]   = {-400, 400, 1020, -1020};
    const uchar   c_o[] = {100, 100, 255, 255};
    abs_c1(64, 0, 4, c, c_o);
    const int16_t d[]   = {0, 5, 10, 20, -20};
    const uchar   d_o[] = {10, 5, 0, 10, 30};
    abs_c1(256, -10, 5, d, d_o);
}

static void test_convert_scale_abs_c3(void) {
    const int16_t in[]  = {-3, 3, -1, 1, 510, -510};
    const uchar   out[] = {2, 2, 1, 1, 255, 255};
    struct Mat   *src   = mat_from_bytes(1, 2, CV_16SC3, in);
    struct Mat   *dst   = alloc_mat(1, 2, CV_8UC3);
    convert_scale_abs(dst, src, 128, 0);
    CHECK_MAT_BYTES(dst, out);
    free_mat(&src);
    free_mat(&dst);
}

// Boundary probes for targets where int is 16 bits. Results are checked
// against closed forms that do not depend on the library's own arithmetic.
static void test_convert_scale_extreme_params(void) {
    // v = i * 32767 + 65535 * 256 + 128 >= 2^24, so every lane saturates high.
    const uchar in[] = {0, 1, 128, 255};
    const uchar hi[] = {255, 255, 255, 255};
    scale_c1(32767, 65535, 4, in, hi);
    // v = -i * 32768 - 65536 * 256 + 128 < 0, so every lane clamps to zero.
    const uchar lo[] = {0, 0, 0, 0};
    scale_c1(-32768, -65536, 4, in, lo);
    // 255 * 32767 / 256 = 32639.0 -> saturates, 1 * 32767 / 256 = 128.
    const uchar mid_in[] = {0, 1, 2};
    const uchar mid_o[]  = {0, 128, 255};
    scale_c1(32767, 0, 3, mid_in, mid_o);
}

static void test_convert_scale_abs_extreme_params(void) {
    const int16_t in[] = {-32768, 32767, 0, -1};
    const uchar   hi[] = {255, 255, 0, 128};
    abs_c1(-32768, 0, 4, in, hi);
    // |-32768 * 32767 / 256 + 65535| and |32767 * -32768 / 256 - 65536| clamp.
    const uchar hi2[] = {255, 255, 255, 255};
    abs_c1(32767, 65535, 4, in, hi2);
    const uchar hi3[] = {255, 255, 255, 255};
    abs_c1(-32768, -65536, 4, in, hi3);
}

static int ref_reflect101(int p, int len) {
    if (len == 1) {
        return 0;
    }
    while (p < 0 || p >= len) {
        p = p < 0 ? -p : 2 * (len - 1) - p;
    }
    return p;
}

// ksize * ksize reaches 33489 here, which does not fit a 16-bit int.
static void check_box_large_kernel(const uchar in[4], int ksize) {
    struct Mat *src = mat_from_bytes(2, 2, CV_8UC1, in);
    struct Mat *dst = alloc_mat(2, 2, CV_8UC1);
    box_filter(dst, src, ksize);

    uchar          expected[4];
    const int      r    = ksize / 2;
    const uint32_t area = (uint32_t)ksize * (uint32_t)ksize;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            uint32_t sum = 0;
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    sum += in[ref_reflect101(y + dy, 2) * 2 +
                              ref_reflect101(x + dx, 2)];
                }
            }
            expected[y * 2 + x] = (uchar)((sum + area / 2) / area);
        }
    }
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_box_filter_large_kernel_area(void) {
    const uchar a[4] = {0, 255, 255, 0};
    const uchar b[4] = {1, 2, 3, 4};
    const uchar c[4] = {255, 255, 255, 255};
    check_box_large_kernel(a, 183);
    check_box_large_kernel(b, 183);
    check_box_large_kernel(c, 183);
    check_box_large_kernel(a, 255);
    check_box_large_kernel(c, 255);
}

static void test_synthetic_large_values(void) {
    // j * 255 passes 32767 once cols > 128.
    struct Mat *m = alloc_mat_synthetic(2, 300);
    CHECK_EQ_INT(MAT_AT_ELEM_PTR(*m, uchar, 0, 299)[0], 255);
    CHECK_EQ_INT(MAT_AT_ELEM_PTR(*m, uchar, 0, 150)[0], 150 * 255 / 299);
    CHECK_EQ_INT(MAT_AT_ELEM_PTR(*m, uchar, 1, 299)[2], 255);
    free_mat(&m);
}

int main(void) {
    RUN_TEST(test_convert_scale_c3);
    RUN_TEST(test_convert_scale_values);
    RUN_TEST(test_convert_scale_identity);
    RUN_TEST(test_convert_scale_abs_values);
    RUN_TEST(test_convert_scale_abs_c3);
    RUN_TEST(test_convert_scale_extreme_params);
    RUN_TEST(test_convert_scale_abs_extreme_params);
    RUN_TEST(test_box_filter_large_kernel_area);
    RUN_TEST(test_synthetic_large_values);
    RUN_TEST(test_lut_invert);
    RUN_TEST(test_lut_identity);
    RUN_TEST(test_lut_constant);
    RUN_TEST(test_lut_width_sweep);
    RUN_TEST(test_threshold_types_c3);
    RUN_TEST(test_threshold_types_c1);
    RUN_TEST(test_threshold_c4_alpha_normal);
    RUN_TEST(test_threshold_extremes);
    RUN_TEST(test_threshold_width_sweep);
    return test_finish();
}
