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
                         COLOR_HSV2BGR, COLOR_RGB2YUV, COLOR_BGR2YUV,
                         COLOR_YUV2RGB, COLOR_YUV2BGR};
    for (int i = 0; i < 8; i++) {
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
    check_misuse_asserts(COLOR_BGR2YUV);
    check_misuse_asserts(COLOR_YUV2RGB);
}

// ---- YUV --------------------------------------------------------------------

static void test_rgb2yuv_hand_values(void) {
    const uchar in[]  = {255, 0,   0,   0,  255, 0,  0,   0,   255,
                         128, 128, 128, 0,  0,   0,  255, 255, 255,
                         200, 100, 50,  10, 20,  30, 255, 255, 0};
    const uchar out[] = {77,  85,  255, 149, 43,  21,  29,  255, 107,
                         128, 128, 128, 0,   128, 128, 255, 128, 128,
                         124, 86,  182, 18,  135, 122, 226, 1,   149};
    convert_bytes(COLOR_RGB2YUV, CV_8UC3, CV_8UC3, 1, 9, in, out);
}

static void test_bgr2yuv_swaps_input(void) {
    const uchar in[]  = {0, 0, 255, 50, 100, 200};
    const uchar out[] = {77, 85, 255, 124, 86, 182};
    convert_bytes(COLOR_BGR2YUV, CV_8UC3, CV_8UC3, 1, 2, in, out);
}

static void test_yuv2rgb_hand_values(void) {
    const uchar in[]  = {128, 128, 128, 77, 85,  255, 100, 200, 50,  29, 255,
                         107, 150, 44,  21, 255, 128, 128, 0,   128, 128};
    const uchar out[] = {128, 128, 128, 255, 1,   1,   0,   131, 228, 0, 0,
                         254, 0,   255, 1,   255, 255, 255, 0,   0,   0};
    convert_bytes(COLOR_YUV2RGB, CV_8UC3, CV_8UC3, 1, 7, in, out);
}

static void test_yuv2bgr_swaps_output(void) {
    const uchar in[]  = {100, 200, 50};
    const uchar out[] = {228, 131, 0};
    convert_bytes(COLOR_YUV2BGR, CV_8UC3, CV_8UC3, 1, 1, in, out);
}

// 64-bit reference with plain arithmetic division by 256 after clamping.
static void ref_rgb2yuv(int r, int g, int b, int out[3]) {
    int64_t y = (77 * r + 150 * g + 29 * b + 128) / 256;
    int64_t u = (32896 - 43 * r - 85 * g + 128 * b) / 256;
    int64_t v = (32896 + 128 * r - 107 * g - 21 * b) / 256;
    out[0]    = (int)y;
    out[1]    = (int)(u > 255 ? 255 : u);
    out[2]    = (int)(v > 255 ? 255 : v);
}

static int ref_yuv_clamp(int64_t x) {
    if (x < 0) {
        return 0;
    }
    x /= 256;
    return (int)(x > 255 ? 255 : x);
}

static void ref_yuv2rgb(int y, int u, int v, int out[3]) {
    out[0] = ref_yuv_clamp(256 * y + 359 * (v - 128) + 128);
    out[1] = ref_yuv_clamp(256 * y - 88 * (u - 128) - 183 * (v - 128) + 128);
    out[2] = ref_yuv_clamp(256 * y + 454 * (u - 128) + 128);
}

static void test_rgb2yuv_grid_matches_reference(void) {
    for (int r = 0; r < 256; r += 3) {
        for (int g = 0; g < 256; g += 3) {
            for (int b = 0; b < 256; b += 3) {
                uchar in[3] = {(uchar)r, (uchar)g, (uchar)b};
                uchar out[3];
                int   want[3];
                convert_px(COLOR_RGB2YUV, in, out);
                ref_rgb2yuv(r, g, b, want);
                if (out[0] != want[0] || out[1] != want[1] ||
                    out[2] != want[2]) {
                    TEST_FAIL_("rgb2yuv(%d,%d,%d) = %d,%d,%d want %d,%d,%d", r,
                               g, b, out[0], out[1], out[2], want[0], want[1],
                               want[2]);
                    return;
                }
            }
        }
    }
    // The corners hit the clamp value 256 for U (blue) and V (red).
    const uchar blue[3] = {0, 0, 255};
    uchar       out[3];
    convert_px(COLOR_RGB2YUV, blue, out);
    CHECK_EQ_INT(out[1], 255);
}

static void test_yuv2rgb_grid_matches_reference(void) {
    for (int y = 0; y < 256; y += 3) {
        for (int u = 0; u < 256; u += 3) {
            for (int v = 0; v < 256; v += 3) {
                uchar in[3] = {(uchar)y, (uchar)u, (uchar)v};
                uchar out[3];
                int   want[3];
                convert_px(COLOR_YUV2RGB, in, out);
                ref_yuv2rgb(y, u, v, want);
                if (out[0] != want[0] || out[1] != want[1] ||
                    out[2] != want[2]) {
                    TEST_FAIL_("yuv2rgb(%d,%d,%d) = %d,%d,%d want %d,%d,%d", y,
                               u, v, out[0], out[1], out[2], want[0], want[1],
                               want[2]);
                    return;
                }
            }
        }
    }
}

static void test_yuv_round_trip_error(void) {
    int worst = 0;
    for (int r = 0; r < 256; r += 5) {
        for (int g = 0; g < 256; g += 5) {
            for (int b = 0; b < 256; b += 5) {
                uchar in[3] = {(uchar)r, (uchar)g, (uchar)b};
                uchar mid[3], back[3];
                convert_px(COLOR_RGB2YUV, in, mid);
                convert_px(COLOR_YUV2RGB, mid, back);
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
    printf("  yuv round trip max error %d\n", worst);
    CHECK(worst <= 2);
}

static void test_yuv_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src = alloc_mat(3, w, CV_8UC3);
        struct Mat *yuv = alloc_mat(3, w, CV_8UC3);
        struct Mat *bgr = alloc_mat(3, w, CV_8UC3);
        struct Mat *yb  = alloc_mat(3, w, CV_8UC3);
        struct Mat *rgb = alloc_mat(3, w, CV_8UC3);
        struct Mat *rb  = alloc_mat(3, w, CV_8UC3);
        fill_pattern(src, (unsigned)w);
        cvt_color(yuv, src, COLOR_RGB2YUV);
        cvt_color(bgr, src, COLOR_RGB2BGR);
        cvt_color(yb, bgr, COLOR_BGR2YUV);
        cvt_color(rgb, src, COLOR_YUV2RGB); // src read as YUV
        cvt_color(rb, src, COLOR_YUV2BGR);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                const uchar *p = mat_at(src, y, x);
                const uchar *q = mat_at(yuv, y, x);
                int          want[3];
                ref_rgb2yuv(p[0], p[1], p[2], want);
                CHECK_EQ_INT(q[0], want[0]);
                CHECK_EQ_INT(q[1], want[1]);
                CHECK_EQ_INT(q[2], want[2]);
                CHECK_EQ_BYTES(mat_at(yb, y, x), q, 3);
                ref_yuv2rgb(p[0], p[1], p[2], want);
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
        free_mat(&yuv);
        free_mat(&bgr);
        free_mat(&yb);
        free_mat(&rgb);
        free_mat(&rb);
    }
}

// ---- Camera formats: RGB565 and YUV422 (YUYV)
// --------------------------------

// Naive references, written from the bit layout and from the integer BT.601
// constants OpenCV uses (color_yuv.simd.hpp), independent of core/color.c.
static void ref_565(unsigned t, int bgr[3]) {
    bgr[0] = (int)((t << 3) & 0xF8u);
    bgr[1] = (int)((t >> 3) & 0xFCu);
    bgr[2] = (int)((t >> 8) & 0xF8u);
}

static int ref_luma(int r, int g, int b) {
    return (77 * r + 150 * g + 29 * b + 128) >> 8;
}

static int ref_sat_shift20(long long x) {
    if (x < 0) {
        return 0;
    }
    x >>= 20;
    return x > 255 ? 255 : (int)x;
}

static void ref_yuyv(int y, int u, int v, int rgb[3]) {
    long long yy = (long long)(y > 16 ? y - 16 : 0) * 1220542;
    long long h  = 1 << 19;
    long long uu = u - 128;
    long long vv = v - 128;
    rgb[0]       = ref_sat_shift20(yy + h + 1673527 * vv);
    rgb[1]       = ref_sat_shift20(yy + h - 852492 * vv - 409993 * uu);
    rgb[2]       = ref_sat_shift20(yy + h + 2116026 * uu);
}

static void test_bgr565_hand_values(void) {
    // Words 0xF800 (red), 0x07E0 (green), 0x001F (blue), 0xFFFF (white).
    const uchar le[]   = {0x00, 0xF8, 0xE0, 0x07, 0x1F, 0x00, 0xFF, 0xFF};
    const uchar be[]   = {0xF8, 0x00, 0x07, 0xE0, 0x00, 0x1F, 0xFF, 0xFF};
    const uchar bgr[]  = {0, 0, 248, 0, 252, 0, 248, 0, 0, 248, 252, 248};
    const uchar rgb[]  = {248, 0, 0, 0, 252, 0, 0, 0, 248, 248, 252, 248};
    const uchar gray[] = {75, 148, 28, 250};
    convert_bytes(COLOR_BGR5652BGR, CV_8UC2, CV_8UC3, 1, 4, le, bgr);
    convert_bytes(COLOR_BGR5652RGB, CV_8UC2, CV_8UC3, 1, 4, le, rgb);
    convert_bytes(COLOR_BGR5652GRAY, CV_8UC2, CV_8UC1, 1, 4, le, gray);
    convert_bytes(COLOR_BGR565BE2BGR, CV_8UC2, CV_8UC3, 1, 4, be, bgr);
    convert_bytes(COLOR_BGR565BE2RGB, CV_8UC2, CV_8UC3, 1, 4, be, rgb);
    convert_bytes(COLOR_BGR565BE2GRAY, CV_8UC2, CV_8UC1, 1, 4, be, gray);
}

// Every 16-bit word, in both byte orders, against the naive reference.
static void test_bgr565_exhaustive(void) {
    struct Mat *le = alloc_mat(256, 256, CV_8UC2);
    struct Mat *be = alloc_mat(256, 256, CV_8UC2);
    struct Mat *out[6];
    const int   codes[6] = {COLOR_BGR5652BGR,   COLOR_BGR5652RGB,
                            COLOR_BGR5652GRAY,  COLOR_BGR565BE2BGR,
                            COLOR_BGR565BE2RGB, COLOR_BGR565BE2GRAY};
    for (unsigned t = 0; t < 65536u; t++) {
        le->data[2 * t]     = (uchar)(t & 0xFFu);
        le->data[2 * t + 1] = (uchar)(t >> 8);
        be->data[2 * t]     = (uchar)(t >> 8);
        be->data[2 * t + 1] = (uchar)(t & 0xFFu);
    }
    for (int i = 0; i < 6; i++) {
        int gray = (i % 3) == 2;
        out[i]   = alloc_mat(256, 256, gray ? CV_8UC1 : CV_8UC3);
        cvt_color(out[i], i < 3 ? le : be, codes[i]);
    }
    int bad = 0;
    for (unsigned t = 0; t < 65536u && bad == 0; t++) {
        int bgr[3];
        ref_565(t, bgr);
        int g = ref_luma(bgr[2], bgr[1], bgr[0]);
        for (int k = 0; k < 2; k++) {
            const uchar *pb = out[3 * k]->data + 3 * t;
            const uchar *pr = out[3 * k + 1]->data + 3 * t;
            const uchar *pg = out[3 * k + 2]->data + t;
            if (pb[0] != bgr[0] || pb[1] != bgr[1] || pb[2] != bgr[2] ||
                pr[0] != bgr[2] || pr[1] != bgr[1] || pr[2] != bgr[0] ||
                pg[0] != g) {
                TEST_FAIL_("565 word 0x%04X order %d", t, k);
                bad = 1;
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        free_mat(&out[i]);
    }
    free_mat(&le);
    free_mat(&be);
}

static void test_yuyv_hand_values(void) {
    // Values checked against cv2.cvtColor(..., COLOR_YUV2RGB_YUYV), 4.10.0.
    const uchar in[]   = {16, 128, 235, 128, 128, 128, 81,  90,
                          41, 240, 145, 54,  0,   0,   255, 255};
    const uchar rgb[]  = {0,   0,   0,   255, 255, 255, 70,  161,
                          130, 15,  107, 76,  0,   45,  255, 32,
                          167, 255, 203, 0,   0,   255, 225, 20};
    const uchar gray[] = {16, 235, 128, 81, 41, 145, 0, 255};
    uchar       bgr[24];
    for (int i = 0; i < 8; i++) {
        bgr[3 * i]     = rgb[3 * i + 2];
        bgr[3 * i + 1] = rgb[3 * i + 1];
        bgr[3 * i + 2] = rgb[3 * i];
    }
    convert_bytes(COLOR_YUV2RGB_YUYV, CV_8UC2, CV_8UC3, 1, 8, in, rgb);
    convert_bytes(COLOR_YUV2BGR_YUYV, CV_8UC2, CV_8UC3, 1, 8, in, bgr);
    convert_bytes(COLOR_YUV2GRAY_YUYV, CV_8UC2, CV_8UC1, 1, 8, in, gray);
}

// All Y values against a U/V grid (step 3, both ends included).
static void test_yuyv_grid_matches_reference(void) {
    struct Mat *src = alloc_mat(1, 256, CV_8UC2);
    struct Mat *rgb = alloc_mat(1, 256, CV_8UC3);
    int         bad = 0;
    for (int u = 0; u < 256 && !bad; u += 3) {
        for (int v = 0; v < 256 && !bad; v += 3) {
            for (int x = 0; x < 256; x++) {
                src->data[2 * x]     = (uchar)x;
                src->data[2 * x + 1] = (uchar)((x & 1) ? v : u);
            }
            cvt_color(rgb, src, COLOR_YUV2RGB_YUYV);
            for (int x = 0; x < 256 && !bad; x++) {
                int want[3];
                ref_yuyv(x, u, v, want);
                const uchar *p = rgb->data + 3 * x;
                if (p[0] != want[0] || p[1] != want[1] || p[2] != want[2]) {
                    TEST_FAIL_("yuyv y=%d u=%d v=%d: got %d %d %d, want %d %d "
                               "%d",
                               x, u, v, p[0], p[1], p[2], want[0], want[1],
                               want[2]);
                    bad = 1;
                }
            }
        }
    }
    free_mat(&src);
    free_mat(&rgb);
}

// Widths 0..65 (565) and even widths 0..64 (YUYV), 3 rows, pattern input.
static void test_camera_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src = alloc_mat(3, w, CV_8UC2);
        struct Mat *c3  = alloc_mat(3, w, CV_8UC3);
        struct Mat *c1  = alloc_mat(3, w, CV_8UC1);
        fill_pattern(src, (unsigned)w);
        for (int order = 0; order < 2; order++) {
            cvt_color(c3, src, order ? COLOR_BGR565BE2RGB : COLOR_BGR5652RGB);
            cvt_color(c1, src, order ? COLOR_BGR565BE2GRAY : COLOR_BGR5652GRAY);
            for (int y = 0; y < 3; y++) {
                for (int x = 0; x < w; x++) {
                    const uchar *s = mat_at(src, y, x);
                    unsigned     t = order ? ((unsigned)s[0] << 8 | s[1])
                                           : ((unsigned)s[1] << 8 | s[0]);
                    int          bgr[3];
                    ref_565(t, bgr);
                    const uchar *p = mat_at(c3, y, x);
                    CHECK_EQ_INT(p[0], bgr[2]);
                    CHECK_EQ_INT(p[1], bgr[1]);
                    CHECK_EQ_INT(p[2], bgr[0]);
                    CHECK_EQ_INT(*(const uchar *)mat_at(c1, y, x),
                                 ref_luma(bgr[2], bgr[1], bgr[0]));
                }
            }
        }
        if (w % 2 == 0) {
            cvt_color(c3, src, COLOR_YUV2RGB_YUYV);
            cvt_color(c1, src, COLOR_YUV2GRAY_YUYV);
            for (int y = 0; y < 3; y++) {
                for (int x = 0; x < w; x++) {
                    const uchar *s    = mat_at(src, y, x);
                    const uchar *pair = mat_at(src, y, x & ~1);
                    int          want[3];
                    ref_yuyv(s[0], pair[1], pair[3], want);
                    CHECK_EQ_BYTES(mat_at(c1, y, x), s, 1);
                    const uchar *p = mat_at(c3, y, x);
                    CHECK_EQ_INT(p[0], want[0]);
                    CHECK_EQ_INT(p[1], want[1]);
                    CHECK_EQ_INT(p[2], want[2]);
                }
            }
        }
        free_mat(&src);
        free_mat(&c3);
        free_mat(&c1);
    }
}

// Zero rows or zero cols return without touching dst, as for the other codes.
static void test_camera_zero_size_leaves_dst(void) {
    const int codes[] = {
        COLOR_BGR5652BGR,   COLOR_BGR5652RGB,   COLOR_BGR5652GRAY,
        COLOR_BGR565BE2BGR, COLOR_BGR565BE2RGB, COLOR_BGR565BE2GRAY,
        COLOR_YUV2RGB_YUYV, COLOR_YUV2BGR_YUYV, COLOR_YUV2GRAY_YUYV};
    for (int i = 0; i < 9; i++) {
        int dtype = (i == 2 || i == 5 || i == 8) ? CV_8UC1 : CV_8UC3;
        for (int mode = 0; mode < 2; mode++) {
            uchar      sbuf[4] = {1, 2, 3, 4};
            uchar      dbuf[6] = {7, 8, 9, 10, 11, 12};
            struct Mat s0, d0;
            mat_init(&s0, 1, 2, CV_8UC2, sbuf, sizeof(sbuf));
            mat_init(&d0, 1, 2, dtype, dbuf, sizeof(dbuf));
            if (mode == 0) {
                s0.cols = d0.cols = 0;
            } else {
                s0.rows = d0.rows = 0;
            }
            cvt_color(&d0, &s0, codes[i]);
            for (int k = 0; k < 6; k++) {
                CHECK_EQ_INT(dbuf[k], 7 + k);
            }
        }
    }
}

static int camera_assert_fires(int code, int stype, int dtype, int cols) {
    struct Mat *src = alloc_mat(2, cols, stype);
    struct Mat *dst = alloc_mat(2, cols, dtype);
    assert_count    = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        cvt_color(dst, src, code);
    }
    improc_set_assert_handler(NULL);
    free_mat(&src);
    free_mat(&dst);
    return assert_count;
}

static void test_camera_misuse_asserts(void) {
    // Source must be CV_8UC2.
    CHECK_EQ_INT(camera_assert_fires(COLOR_BGR5652RGB, CV_8UC3, CV_8UC3, 4), 1);
    CHECK_EQ_INT(camera_assert_fires(COLOR_YUV2GRAY_YUYV, CV_8UC1, CV_8UC1, 4),
                 1);
    // Destination channel count must match.
    CHECK_EQ_INT(camera_assert_fires(COLOR_BGR565BE2GRAY, CV_8UC2, CV_8UC3, 4),
                 1);
    // YUYV needs an even width; 565 does not.
    CHECK_EQ_INT(camera_assert_fires(COLOR_YUV2RGB_YUYV, CV_8UC2, CV_8UC3, 5),
                 1);
    CHECK_EQ_INT(camera_assert_fires(COLOR_YUV2GRAY_YUYV, CV_8UC2, CV_8UC1, 5),
                 1);
    CHECK_EQ_INT(camera_assert_fires(COLOR_BGR5652GRAY, CV_8UC2, CV_8UC1, 5),
                 0);
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
    RUN_TEST(test_rgb2yuv_hand_values);
    RUN_TEST(test_bgr2yuv_swaps_input);
    RUN_TEST(test_yuv2rgb_hand_values);
    RUN_TEST(test_yuv2bgr_swaps_output);
    RUN_TEST(test_rgb2yuv_grid_matches_reference);
    RUN_TEST(test_yuv2rgb_grid_matches_reference);
    RUN_TEST(test_yuv_round_trip_error);
    RUN_TEST(test_yuv_width_sweep);
    RUN_TEST(test_color2_zero_size_leaves_dst);
    RUN_TEST(test_color2_misuse_asserts);
    RUN_TEST(test_bgr565_hand_values);
    RUN_TEST(test_bgr565_exhaustive);
    RUN_TEST(test_yuyv_hand_values);
    RUN_TEST(test_yuyv_grid_matches_reference);
    RUN_TEST(test_camera_width_sweep);
    RUN_TEST(test_camera_zero_size_leaves_dst);
    RUN_TEST(test_camera_misuse_asserts);
    return test_finish();
}
