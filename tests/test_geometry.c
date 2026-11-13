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

int main(void) {
    RUN_TEST(test_flip_c3);
    RUN_TEST(test_flip_gray);
    RUN_TEST(test_flip_rgba);
    RUN_TEST(test_flip_twice_identity);
    RUN_TEST(test_flip_both_is_h_then_v);
    return test_finish();
}
