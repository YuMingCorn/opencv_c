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

int main(void) {
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
