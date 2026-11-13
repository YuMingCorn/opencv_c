#include <stdint.h>

#include "geometry.h"
#include "test_mat_util.h"

static void flip_bytes(int flip_code, int type, int rows, int cols,
                       const uchar *in, const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, type, in);
    struct Mat *dst = alloc_mat(rows, cols, type);
    flip(dst, src, flip_code);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_flip_c3(void) {
    const uchar in[]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    const uchar h[]    = {4, 5, 6, 1, 2, 3, 10, 11, 12, 7, 8, 9};
    const uchar v[]    = {7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6};
    const uchar both[] = {10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3};
    flip_bytes(FLIP_HORIZONTAL, CV_8UC3, 2, 2, in, h);
    flip_bytes(FLIP_VERTICAL, CV_8UC3, 2, 2, in, v);
    flip_bytes(FLIP_BOTH, CV_8UC3, 2, 2, in, both);
}

static void test_flip_gray(void) {
    const uchar in[]  = {1, 2, 3};
    const uchar out[] = {3, 2, 1};
    flip_bytes(FLIP_HORIZONTAL, CV_8UC1, 1, 3, in, out);
}

static void test_flip_rgba(void) {
    const uchar in[]  = {1, 2, 3, 4, 5, 6, 7, 8};
    const uchar out[] = {5, 6, 7, 8, 1, 2, 3, 4};
    flip_bytes(FLIP_HORIZONTAL, CV_8UC4, 1, 2, in, out);
}

static void test_flip_twice_identity(void) {
    const int codes[] = {FLIP_VERTICAL, FLIP_HORIZONTAL, FLIP_BOTH};
    for (int w = 0; w <= 65; w++) {
        for (int c = 0; c < 3; c++) {
            struct Mat *src = alloc_mat(3, w, CV_8UC3);
            struct Mat *mid = alloc_mat(3, w, CV_8UC3);
            struct Mat *dst = alloc_mat(3, w, CV_8UC3);
            fill_pattern(src, (unsigned)w);
            flip(mid, src, codes[c]);
            flip(dst, mid, codes[c]);
            CHECK_EQ_BYTES(dst->data, src->data, (size_t)3 * src->step[0]);
            free_mat(&src);
            free_mat(&mid);
            free_mat(&dst);
        }
    }
}

static void test_flip_both_is_h_then_v(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src  = alloc_mat(3, w, CV_8UC3);
        struct Mat *h    = alloc_mat(3, w, CV_8UC3);
        struct Mat *hv   = alloc_mat(3, w, CV_8UC3);
        struct Mat *both = alloc_mat(3, w, CV_8UC3);
        fill_pattern(src, (unsigned)w);
        flip(h, src, FLIP_HORIZONTAL);
        flip(hv, h, FLIP_VERTICAL);
        flip(both, src, FLIP_BOTH);
        CHECK_EQ_BYTES(both->data, hv->data, (size_t)3 * both->step[0]);
        free_mat(&src);
        free_mat(&h);
        free_mat(&hv);
        free_mat(&both);
    }
}

static void resize_bytes(int interp, int type, int srows, int scols, int drows,
                         int dcols, const uchar *in, const uchar *expected) {
    struct Mat *src = mat_from_bytes(srows, scols, type, in);
    struct Mat *dst = alloc_mat(drows, dcols, type);
    resize(dst, src, interp);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_resize_linear_1d(void) {
    const uchar a[]     = {0, 100};
    const uchar a_out[] = {0, 25, 75, 100};
    resize_bytes(INTER_LINEAR, CV_8UC1, 1, 2, 1, 4, a, a_out);
    const uchar b[]     = {10, 20, 30, 40};
    const uchar b_out[] = {15, 35};
    resize_bytes(INTER_LINEAR, CV_8UC1, 1, 4, 1, 2, b, b_out);
    const uchar c[]     = {0, 90};
    const uchar c_out[] = {0, 45, 90};
    resize_bytes(INTER_LINEAR, CV_8UC1, 1, 2, 1, 3, c, c_out);
    const uchar d[]     = {0, 30, 90};
    const uchar d_out[] = {8, 75};
    resize_bytes(INTER_LINEAR, CV_8UC1, 1, 3, 1, 2, d, d_out);
}

static void test_resize_linear_2d(void) {
    const uchar a[]     = {10, 20, 30, 40};
    const uchar a_out[] = {25};
    resize_bytes(INTER_LINEAR, CV_8UC1, 2, 2, 1, 1, a, a_out);
    const uchar b[]     = {0, 100, 100, 200};
    const uchar b_out[] = {0,  25,  75,  100, 25,  50,  100, 125,
                           75, 100, 150, 175, 100, 125, 175, 200};
    resize_bytes(INTER_LINEAR, CV_8UC1, 2, 2, 4, 4, b, b_out);
    const uchar c[]     = {0, 90, 180, 255};
    const uchar c_out[] = {0, 45, 90, 90, 131, 173, 180, 218, 255};
    resize_bytes(INTER_LINEAR, CV_8UC1, 2, 2, 3, 3, c, c_out);
}

static void test_resize_same_size_identity(void) {
    const uchar in[] = {1, 2, 3, 4, 5, 6};
    resize_bytes(INTER_LINEAR, CV_8UC1, 2, 3, 2, 3, in, in);
    resize_bytes(INTER_NEAREST, CV_8UC1, 2, 3, 2, 3, in, in);
}

static void test_resize_linear_rgb(void) {
    const uchar in[]  = {0, 100, 200, 100, 200, 0};
    const uchar out[] = {0, 100, 200, 25, 125, 150, 75, 175, 50, 100, 200, 0};
    resize_bytes(INTER_LINEAR, CV_8UC3, 1, 2, 1, 4, in, out);
}

static void check_constant_resize(int drows, int dcols) {
    struct Mat *src = alloc_mat(7, 5, CV_8UC1);
    memset(src->data, 7, (size_t)src->rows * src->step[0]);
    struct Mat *dst = alloc_mat(drows, dcols, CV_8UC1);
    resize(dst, src, INTER_LINEAR);
    for (int y = 0; y < dst->rows; y++) {
        for (int x = 0; x < dst->cols; x++) {
            CHECK_EQ_INT(*(uchar *)mat_at(dst, y, x), 7);
        }
    }
    free_mat(&src);
    free_mat(&dst);
}

static void test_resize_linear_constant(void) {
    check_constant_resize(3, 13);
    check_constant_resize(1, 300);
}

static void test_resize_nearest(void) {
    const uchar a[]     = {0, 100};
    const uchar a_out[] = {0, 0, 100, 100};
    resize_bytes(INTER_NEAREST, CV_8UC1, 1, 2, 1, 4, a, a_out);
    const uchar b[]     = {10, 20, 30, 40};
    const uchar b_out[] = {20, 40};
    resize_bytes(INTER_NEAREST, CV_8UC1, 1, 4, 1, 2, b, b_out);
    const uchar c[]     = {1, 2, 3, 4};
    const uchar c_out[] = {1, 1, 2, 2, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 4, 4};
    resize_bytes(INTER_NEAREST, CV_8UC1, 2, 2, 4, 4, c, c_out);
    const uchar d[]     = {1, 2, 3};
    const uchar d_out[] = {1, 3};
    resize_bytes(INTER_NEAREST, CV_8UC1, 1, 3, 1, 2, d, d_out);
    const uchar e[]     = {1, 2, 3, 4, 5};
    const uchar e_out[] = {1, 3, 5};
    resize_bytes(INTER_NEAREST, CV_8UC1, 1, 5, 1, 3, e, e_out);
    const uchar f[]     = {1, 2};
    const uchar f_out[] = {1, 2, 2};
    resize_bytes(INTER_NEAREST, CV_8UC1, 1, 2, 1, 3, f, f_out);
}

static void test_resize_nearest_sweep(void) {
    struct Mat *src = alloc_mat(1, 7, CV_8UC3);
    fill_pattern(src, 5);
    for (int dw = 1; dw <= 65; dw++) {
        struct Mat *dst = alloc_mat(1, dw, CV_8UC3);
        resize(dst, src, INTER_NEAREST);
        for (int x = 0; x < dw; x++) {
            int          sx = (int)(((int64_t)(2 * x + 1) * 7) / (2 * dw));
            const uchar *p  = mat_at(src, 0, sx);
            const uchar *q  = mat_at(dst, 0, x);
            for (int c = 0; c < 3; c++) {
                CHECK_EQ_INT(q[c], p[c]);
            }
        }
        free_mat(&dst);
    }
    free_mat(&src);
}

int main(void) {
    RUN_TEST(test_flip_c3);
    RUN_TEST(test_flip_gray);
    RUN_TEST(test_flip_rgba);
    RUN_TEST(test_flip_twice_identity);
    RUN_TEST(test_flip_both_is_h_then_v);
    RUN_TEST(test_resize_linear_1d);
    RUN_TEST(test_resize_linear_2d);
    RUN_TEST(test_resize_same_size_identity);
    RUN_TEST(test_resize_linear_rgb);
    RUN_TEST(test_resize_linear_constant);
    RUN_TEST(test_resize_nearest);
    RUN_TEST(test_resize_nearest_sweep);
    return test_finish();
}
