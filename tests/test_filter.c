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

int main(void) {
    RUN_TEST(test_box_impulse);
    RUN_TEST(test_box_ramp);
    RUN_TEST(test_box_rounding);
    RUN_TEST(test_box_k5_wide_border);
    RUN_TEST(test_box_kernel_larger_than_image);
    RUN_TEST(test_box_rgb);
    RUN_TEST(test_box_ksize1_identity);
    RUN_TEST(test_box_width_sweep);
    return test_finish();
}
