#include <setjmp.h>
#include <stdint.h>

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

// ---- HSV --------------------------------------------------------------------

static jmp_buf assert_jump;
static int     assert_count;

static void jump_handler(const char *expr, const char *file, int line) {
    (void)expr;
    (void)file;
    (void)line;
    assert_count++;
    longjmp(assert_jump, 1);
}

static void test_rgb2hsv_hand_values(void) {
    const uchar in[]  = {255, 0,   0,   0,   255, 0,  0,   0,   255, 255, 255,
                         0,   128, 128, 128, 0,   0,  0,   255, 255, 255, 200,
                         100, 50,  10,  20,  30,  50, 100, 200, 200, 50,  100};
    const uchar out[] = {0,   255, 255, 60,  255, 255, 120, 255, 255, 30,  255,
                         255, 0,   0,   128, 0,   0,   0,   0,   0,   255, 10,
                         191, 200, 105, 170, 30,  110, 191, 200, 170, 191, 200};
    convert_bytes(COLOR_RGB2HSV, CV_8UC3, CV_8UC3, 1, 11, in, out);
}

static void test_bgr2hsv_swaps_input(void) {
    // BGR (0,0,255) is RGB red; BGR (50,100,200) is RGB (200,100,50).
    const uchar in[]  = {0, 0, 255, 50, 100, 200};
    const uchar out[] = {0, 255, 255, 10, 191, 200};
    convert_bytes(COLOR_BGR2HSV, CV_8UC3, CV_8UC3, 1, 2, in, out);
}

static void test_hsv2rgb_hand_values(void) {
    const uchar in[]  = {0,   255, 255, 60,  255, 255, 120, 255,
                         255, 15,  128, 200, 90,  100, 50,  45,
                         200, 100, 179, 255, 255, 200, 255, 255};
    const uchar out[] = {255, 0,  0,  0,  255, 0,  0,   0, 255, 200, 150, 100,
                         30,  50, 50, 61, 100, 22, 255, 0, 9,   255, 170, 0};
    convert_bytes(COLOR_HSV2RGB, CV_8UC3, CV_8UC3, 1, 8, in, out);
}

static void test_hsv2bgr_swaps_output(void) {
    const uchar in[]  = {15, 128, 200};
    const uchar out[] = {100, 150, 200};
    convert_bytes(COLOR_HSV2BGR, CV_8UC3, CV_8UC3, 1, 1, in, out);
}

// Reference without tables: the same fixed-point formulas, divisions inline,
// 64-bit arithmetic.
static void ref_rgb2hsv(int r, int g, int b, int out[3]) {
    int v = r, mn = r;
    if (g > v)
        v = g;
    if (b > v)
        v = b;
    if (g < mn)
        mn = g;
    if (b < mn)
        mn = b;
    int64_t diff = v - mn;
    int64_t sdiv = v ? (2 * 1044480 + v) / (2 * v) : 0;
    int64_t hdiv = diff ? (2 * 122880 + diff) / (2 * diff) : 0;
    int64_t s    = (diff * sdiv + 2048) / 4096;
    int64_t hn   = (v == r)   ? g - b
                   : (v == g) ? b - r + 2 * diff
                              : r - g + 4 * diff;
    int64_t x    = hn * hdiv + 2048;
    int64_t h    = x >= 0 ? x / 4096 : -((-x + 4095) / 4096);
    if (h < 0)
        h += 180;
    out[0] = (int)h;
    out[1] = (int)s;
    out[2] = v;
}

static void ref_hsv2rgb(int h, int s, int v, int out[3]) {
    h %= 180;
    int64_t sec = h / 30, f = h - 30 * sec;
    int64_t val[4];
    val[0] = v;
    val[1] = ((int64_t)v * (7650 - s * (30 - f)) + 3825) / 7650; // t
    val[2] = ((int64_t)v * (255 - s) + 127) / 255;               // p
    val[3] = ((int64_t)v * (7650 - s * f) + 3825) / 7650;        // q
    // Per sector: indexes into val for (R, G, B).
    static const int tbl[6][3] = {{0, 1, 2}, {3, 0, 2}, {2, 0, 1},
                                  {2, 3, 0}, {1, 2, 0}, {0, 2, 3}};
    for (int c = 0; c < 3; c++) {
        out[c] = (int)val[tbl[sec][c]];
    }
}

static void convert_px(int code, const uchar in[3], uchar out[3]) {
    uchar      a[3] = {in[0], in[1], in[2]};
    struct Mat s, d;
    mat_init(&s, 1, 1, CV_8UC3, a, 3);
    mat_init(&d, 1, 1, CV_8UC3, out, 3);
    cvt_color(&d, &s, code);
}

static void test_rgb2hsv_grid_matches_reference(void) {
    for (int r = 0; r < 256; r += 3) {
        for (int g = 0; g < 256; g += 3) {
            for (int b = 0; b < 256; b += 3) {
                uchar in[3] = {(uchar)r, (uchar)g, (uchar)b};
                uchar out[3];
                int   want[3];
                convert_px(COLOR_RGB2HSV, in, out);
                ref_rgb2hsv(r, g, b, want);
                if (out[0] != want[0] || out[1] != want[1] ||
                    out[2] != want[2]) {
                    TEST_FAIL_("rgb2hsv(%d,%d,%d) = %d,%d,%d want %d,%d,%d", r,
                               g, b, out[0], out[1], out[2], want[0], want[1],
                               want[2]);
                    return;
                }
            }
        }
    }
}

static void test_hsv2rgb_grid_matches_reference(void) {
    // Includes H 180..255, which the kernel wraps modulo 180.
    for (int h = 0; h < 256; h++) {
        for (int s = 0; s < 256; s += 5) {
            for (int v = 0; v < 256; v += 5) {
                uchar in[3] = {(uchar)h, (uchar)s, (uchar)v};
                uchar out[3];
                int   want[3];
                convert_px(COLOR_HSV2RGB, in, out);
                ref_hsv2rgb(h, s, v, want);
                if (out[0] != want[0] || out[1] != want[1] ||
                    out[2] != want[2]) {
                    TEST_FAIL_("hsv2rgb(%d,%d,%d) = %d,%d,%d want %d,%d,%d", h,
                               s, v, out[0], out[1], out[2], want[0], want[1],
                               want[2]);
                    return;
                }
            }
        }
    }
}

static void test_hsv_round_trip_error(void) {
    int worst = 0;
    for (int r = 0; r < 256; r += 5) {
        for (int g = 0; g < 256; g += 5) {
            for (int b = 0; b < 256; b += 5) {
                uchar in[3] = {(uchar)r, (uchar)g, (uchar)b};
                uchar mid[3], back[3];
                convert_px(COLOR_RGB2HSV, in, mid);
                convert_px(COLOR_HSV2RGB, mid, back);
                for (int k = 0; k < 3; k++) {
                    int e = (int)in[k] - (int)back[k];
                    if (e < 0)
                        e = -e;
                    if (e > worst)
                        worst = e;
                }
            }
        }
    }
    printf("  hsv round trip max error %d\n", worst);
    CHECK(worst <= 5);
}

static void test_hsv_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src = alloc_mat(3, w, CV_8UC3);
        struct Mat *hsv = alloc_mat(3, w, CV_8UC3);
        struct Mat *bgr = alloc_mat(3, w, CV_8UC3);
        struct Mat *hb  = alloc_mat(3, w, CV_8UC3);
        struct Mat *rgb = alloc_mat(3, w, CV_8UC3);
        struct Mat *rb  = alloc_mat(3, w, CV_8UC3);
        fill_pattern(src, (unsigned)w);
        cvt_color(hsv, src, COLOR_RGB2HSV);
        cvt_color(bgr, src, COLOR_RGB2BGR);
        cvt_color(hb, bgr, COLOR_BGR2HSV);
        cvt_color(rgb, hsv, COLOR_HSV2RGB);
        cvt_color(rb, hsv, COLOR_HSV2BGR);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                const uchar *p = mat_at(src, y, x);
                const uchar *q = mat_at(hsv, y, x);
                int          want[3];
                ref_rgb2hsv(p[0], p[1], p[2], want);
                CHECK_EQ_INT(q[0], want[0]);
                CHECK_EQ_INT(q[1], want[1]);
                CHECK_EQ_INT(q[2], want[2]);
                CHECK_EQ_BYTES(mat_at(hb, y, x), q, 3);
                ref_hsv2rgb(q[0], q[1], q[2], want);
                const uchar *r = mat_at(rgb, y, x);
                const uchar *o = mat_at(rb, y, x);
                CHECK_EQ_INT(r[0], want[0]);
                CHECK_EQ_INT(r[1], want[1]);
                CHECK_EQ_INT(r[2], want[2]);
                CHECK_EQ_INT(o[0], want[2]);
                CHECK_EQ_INT(o[1], want[1]);
                CHECK_EQ_INT(o[2], want[0]);
            }
        }
        free_mat(&src);
        free_mat(&hsv);
        free_mat(&bgr);
        free_mat(&hb);
        free_mat(&rgb);
        free_mat(&rb);
    }
}

// Zero-sized inputs return without touching dst. dst is a header over a caller
// buffer holding sentinel bytes, with cols forced to 0 or rows to 0.
static void test_color2_zero_size_leaves_dst(void) {
    const int codes[] = {COLOR_RGB2HSV, COLOR_BGR2HSV, COLOR_HSV2RGB,
                         COLOR_HSV2BGR};
    for (int i = 0; i < 4; i++) {
        for (int mode = 0; mode < 2; mode++) {
            uchar      sbuf[3] = {1, 2, 3};
            uchar      dbuf[3] = {7, 8, 9};
            struct Mat s0, d0;
            mat_init(&s0, 1, 1, CV_8UC3, sbuf, 3);
            mat_init(&d0, 1, 1, CV_8UC3, dbuf, 3);
            if (mode == 0) {
                s0.cols = d0.cols = 0;
            } else {
                s0.rows = d0.rows = 0;
            }
            cvt_color(&d0, &s0, codes[i]);
            CHECK_EQ_INT(dbuf[0], 7);
            CHECK_EQ_INT(dbuf[1], 8);
            CHECK_EQ_INT(dbuf[2], 9);
        }
    }
}

static void check_misuse_asserts(int code) {
    struct Mat *src3 = alloc_mat(2, 2, CV_8UC3);
    struct Mat *dst1 = alloc_mat(2, 2, CV_8UC1);
    struct Mat *dst3 = alloc_mat(3, 2, CV_8UC3);
    assert_count     = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        cvt_color(dst1, src3, code);
    }
    CHECK_EQ_INT(assert_count, 1);
    if (setjmp(assert_jump) == 0) {
        cvt_color(dst3, src3, code);
    }
    CHECK_EQ_INT(assert_count, 2);
    improc_set_assert_handler(NULL);
    free_mat(&src3);
    free_mat(&dst1);
    free_mat(&dst3);
}

static void test_color2_misuse_asserts(void) {
    check_misuse_asserts(COLOR_RGB2HSV);
    check_misuse_asserts(COLOR_HSV2BGR);
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
    RUN_TEST(test_rgb2hsv_hand_values);
    RUN_TEST(test_bgr2hsv_swaps_input);
    RUN_TEST(test_hsv2rgb_hand_values);
    RUN_TEST(test_hsv2bgr_swaps_output);
    RUN_TEST(test_rgb2hsv_grid_matches_reference);
    RUN_TEST(test_hsv2rgb_grid_matches_reference);
    RUN_TEST(test_hsv_round_trip_error);
    RUN_TEST(test_hsv_width_sweep);
    RUN_TEST(test_color2_zero_size_leaves_dst);
    RUN_TEST(test_color2_misuse_asserts);
    return test_finish();
}
