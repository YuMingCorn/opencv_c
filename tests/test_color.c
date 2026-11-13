#include "color.h"
#include "test_mat_util.h"

static void convert_bytes(int code, int stype, int dtype, int rows, int cols,
                          const uchar *in, const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, stype, in);
    struct Mat *dst = alloc_mat(rows, cols, dtype);
    cvt_color(dst, src, code);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_rgb2gray_primaries(void) {
    const uchar in[]  = {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255};
    const uchar out[] = {77, 149, 29, 255};
    convert_bytes(COLOR_RGB2GRAY, CV_8UC3, CV_8UC1, 2, 2, in, out);
}

static void test_rgb_bgr_gray_values(void) {
    const uchar in[]      = {100, 150, 200, 10, 20, 30, 12, 34, 56};
    const uchar rgb_out[] = {141, 18, 30};
    const uchar bgr_out[] = {159, 22, 38};
    convert_bytes(COLOR_RGB2GRAY, CV_8UC3, CV_8UC1, 1, 3, in, rgb_out);
    convert_bytes(COLOR_BGR2GRAY, CV_8UC3, CV_8UC1, 1, 3, in, bgr_out);
}

static void test_gray_rounding_probes(void) {
    const uchar in[]  = {1, 1, 1, 0, 0, 1, 2, 0, 0, 255, 0, 255};
    const uchar out[] = {1, 0, 1, 106};
    convert_bytes(COLOR_RGB2GRAY, CV_8UC3, CV_8UC1, 1, 4, in, out);
}

static void test_rgb2gray_synthetic(void) {
    struct Mat *src = alloc_mat_synthetic(3, 3);
    struct Mat *dst = alloc_mat(3, 3, CV_8UC1);
    cvt_color(dst, src, COLOR_RGB2GRAY);
    const uchar expected[] = {0, 45, 91, 82, 127, 173, 164, 209, 255};
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_rgba2gray(void) {
    const uchar in[]  = {100, 150, 200, 7};
    const uchar out[] = {141};
    convert_bytes(COLOR_RGBA2GRAY, CV_8UC4, CV_8UC1, 1, 1, in, out);
}

static void test_bgra2gray(void) {
    const uchar in[]  = {200, 150, 100, 7};
    const uchar out[] = {141};
    convert_bytes(COLOR_BGRA2GRAY, CV_8UC4, CV_8UC1, 1, 1, in, out);
}

static void test_reorder_and_alpha(void) {
    const uchar c3[] = {1, 2, 3};
    const uchar c4[] = {1, 2, 3, 4};
    const uchar g[]  = {9};
    const uchar o1[] = {3, 2, 1};
    const uchar o2[] = {3, 2, 1, 4};
    const uchar o3[] = {1, 2, 3, 255};
    const uchar o4[] = {1, 2, 3};
    const uchar o5[] = {3, 2, 1, 255};
    const uchar o6[] = {9, 9, 9};
    const uchar o7[] = {9, 9, 9, 255};
    convert_bytes(COLOR_RGB2BGR, CV_8UC3, CV_8UC3, 1, 1, c3, o1);
    convert_bytes(COLOR_RGBA2BGRA, CV_8UC4, CV_8UC4, 1, 1, c4, o2);
    convert_bytes(COLOR_RGB2RGBA, CV_8UC3, CV_8UC4, 1, 1, c3, o3);
    convert_bytes(COLOR_RGBA2RGB, CV_8UC4, CV_8UC3, 1, 1, c4, o4);
    convert_bytes(COLOR_RGB2BGRA, CV_8UC3, CV_8UC4, 1, 1, c3, o5);
    convert_bytes(COLOR_RGBA2BGR, CV_8UC4, CV_8UC3, 1, 1, c4, o1);
    convert_bytes(COLOR_GRAY2RGB, CV_8UC1, CV_8UC3, 1, 1, g, o6);
    convert_bytes(COLOR_GRAY2RGBA, CV_8UC1, CV_8UC4, 1, 1, g, o7);
}

static void test_aliases(void) {
    CHECK_EQ_INT(COLOR_BGR2RGB, COLOR_RGB2BGR);
    CHECK_EQ_INT(COLOR_BGRA2RGBA, COLOR_RGBA2BGRA);
    CHECK_EQ_INT(COLOR_BGR2BGRA, COLOR_RGB2RGBA);
    CHECK_EQ_INT(COLOR_BGRA2BGR, COLOR_RGBA2RGB);
    CHECK_EQ_INT(COLOR_BGR2RGBA, COLOR_RGB2BGRA);
    CHECK_EQ_INT(COLOR_BGRA2RGB, COLOR_RGBA2BGR);
    CHECK_EQ_INT(COLOR_GRAY2BGR, COLOR_GRAY2RGB);
    CHECK_EQ_INT(COLOR_GRAY2BGRA, COLOR_GRAY2RGBA);
}

static void test_round_trips(void) {
    struct Mat *src = alloc_mat(4, 5, CV_8UC3);
    fill_pattern(src, 3);
    struct Mat *mid  = alloc_mat(4, 5, CV_8UC3);
    struct Mat *back = alloc_mat(4, 5, CV_8UC3);
    cvt_color(mid, src, COLOR_RGB2BGR);
    cvt_color(back, mid, COLOR_BGR2RGB);
    CHECK_MAT_BYTES(back, src->data);
    free_mat(&mid);
    free_mat(&back);

    struct Mat *rgba = alloc_mat(4, 5, CV_8UC4);
    back             = alloc_mat(4, 5, CV_8UC3);
    cvt_color(rgba, src, COLOR_RGB2RGBA);
    cvt_color(back, rgba, COLOR_RGBA2RGB);
    CHECK_MAT_BYTES(back, src->data);
    free_mat(&rgba);
    free_mat(&back);
    free_mat(&src);
}

static void test_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src  = alloc_mat(3, w, CV_8UC3);
        struct Mat *gray = alloc_mat(3, w, CV_8UC1);
        struct Mat *bgr  = alloc_mat(3, w, CV_8UC3);
        fill_pattern(src, (unsigned)w);
        cvt_color(gray, src, COLOR_RGB2GRAY);
        cvt_color(bgr, src, COLOR_RGB2BGR);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                const uchar *p = mat_at(src, y, x);
                unsigned     v =
                    (77u * p[0] + 150u * p[1] + 29u * p[2] + 128u) >> 8;
                CHECK_EQ_INT(*(uchar *)mat_at(gray, y, x), v);
                const uchar *q = mat_at(bgr, y, x);
                CHECK_EQ_INT(q[0], p[2]);
                CHECK_EQ_INT(q[1], p[1]);
                CHECK_EQ_INT(q[2], p[0]);
            }
        }
        free_mat(&src);
        free_mat(&gray);
        free_mat(&bgr);
    }
}

int main(void) {
    RUN_TEST(test_rgb2gray_primaries);
    RUN_TEST(test_rgb_bgr_gray_values);
    RUN_TEST(test_gray_rounding_probes);
    RUN_TEST(test_rgb2gray_synthetic);
    RUN_TEST(test_rgba2gray);
    RUN_TEST(test_bgra2gray);
    RUN_TEST(test_reorder_and_alpha);
    RUN_TEST(test_aliases);
    RUN_TEST(test_round_trips);
    RUN_TEST(test_width_sweep);
    return test_finish();
}
