#include <setjmp.h>
#include <stdint.h>

#include "geometry.h"
#include "test_alloc_util.h"
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

// ---- INTER_AREA -----------------------------------------------------------

// Overlap of destination interval [d*s_len, (d+1)*s_len) and source interval
// [k*d_len, (k+1)*d_len), in units of 1/d_len.
static uint64_t area_weight(uint64_t d, uint64_t k, uint64_t s_len,
                            uint64_t d_len) {
    uint64_t lo = d * s_len > k * d_len ? d * s_len : k * d_len;
    uint64_t hi =
        (d + 1) * s_len < (k + 1) * d_len ? (d + 1) * s_len : (k + 1) * d_len;
    return hi > lo ? hi - lo : 0;
}

static void test_area_weight_tables(void) {
    static const uint8_t w3_2[2][3] = {{2, 1, 0}, {0, 1, 2}};
    static const uint8_t w5_2[2][5] = {{2, 2, 1, 0, 0}, {0, 0, 1, 2, 2}};
    static const uint8_t w4_3[3][4] = {
        {3, 1, 0, 0}, {0, 2, 2, 0}, {0, 0, 1, 3}};
    static const uint8_t w2_3[3][2] = {{2, 0}, {1, 1}, {0, 2}};
    for (uint64_t d = 0; d < 2; d++) {
        for (uint64_t k = 0; k < 3; k++) {
            CHECK_EQ_INT(area_weight(d, k, 3, 2), w3_2[d][k]);
        }
        for (uint64_t k = 0; k < 5; k++) {
            CHECK_EQ_INT(area_weight(d, k, 5, 2), w5_2[d][k]);
        }
    }
    for (uint64_t d = 0; d < 3; d++) {
        for (uint64_t k = 0; k < 4; k++) {
            CHECK_EQ_INT(area_weight(d, k, 4, 3), w4_3[d][k]);
        }
        for (uint64_t k = 0; k < 2; k++) {
            CHECK_EQ_INT(area_weight(d, k, 2, 3), w2_3[d][k]);
        }
    }
}

static void test_area_hand_cases(void) {
    const uchar a[]     = {0,  10, 20,  30,  40,  50,  60,  70,
                           80, 90, 100, 110, 120, 130, 140, 150};
    const uchar a_out[] = {25, 45, 105, 125};
    resize_bytes(INTER_AREA, CV_8UC1, 4, 4, 2, 2, a, a_out);

    const uchar b[]     = {10, 20, 40};
    const uchar b_out[] = {13, 33};
    resize_bytes(INTER_AREA, CV_8UC1, 1, 3, 1, 2, b, b_out);

    const uchar c[]     = {0, 10, 20, 30, 40};
    const uchar c_out[] = {8, 32};
    resize_bytes(INTER_AREA, CV_8UC1, 1, 5, 1, 2, c, c_out);

    const uchar d[]     = {0, 100, 200, 255};
    const uchar d_out[] = {25, 150, 241};
    resize_bytes(INTER_AREA, CV_8UC1, 1, 4, 1, 3, d, d_out);

    const uchar e[]     = {0, 30, 60, 90, 120, 150, 180, 210, 240};
    const uchar e_out[] = {40, 80, 160, 200};
    resize_bytes(INTER_AREA, CV_8UC1, 3, 3, 2, 2, e, e_out);

    const uchar f[]     = {1, 2, 3, 4};
    const uchar f_out[] = {3};
    resize_bytes(INTER_AREA, CV_8UC1, 2, 2, 1, 1, f, f_out);

    const uchar g[]     = {0, 90};
    const uchar g_out[] = {0, 45, 90};
    resize_bytes(INTER_AREA, CV_8UC1, 1, 2, 1, 3, g, g_out);

    const uchar h[]     = {10, 50};
    const uchar h_out[] = {10, 10, 50, 50};
    resize_bytes(INTER_AREA, CV_8UC1, 1, 2, 1, 4, h, h_out);

    const uchar i[] = {7, 8, 9, 10, 11, 12};
    resize_bytes(INTER_AREA, CV_8UC1, 2, 3, 2, 3, i, i);
}

static void test_area_rgb_channels_independent(void) {
    // Channels carry the 1D cases [10,20,40], [0,10,20] and constant 9.
    const uchar in[]  = {10, 0, 9, 20, 10, 9, 40, 20, 9};
    const uchar out[] = {13, 3, 9, 33, 17, 9};
    resize_bytes(INTER_AREA, CV_8UC3, 1, 3, 1, 2, in, out);
}

static void area_reference(struct Mat *dst, const struct Mat *src) {
    uint64_t sw = (uint64_t)src->cols, sh = (uint64_t)src->rows;
    uint64_t dw = (uint64_t)dst->cols, dh = (uint64_t)dst->rows;
    int      cn  = (int)mat_channel_num(src);
    uint64_t den = sw * sh;
    for (uint64_t y = 0; y < dh; y++) {
        for (uint64_t x = 0; x < dw; x++) {
            for (int c = 0; c < cn; c++) {
                uint64_t acc = 0;
                for (uint64_t sy = 0; sy < sh; sy++) {
                    uint64_t wy = area_weight(y, sy, sh, dh);
                    if (wy == 0) {
                        continue;
                    }
                    for (uint64_t sx = 0; sx < sw; sx++) {
                        uint64_t wx = area_weight(x, sx, sw, dw);
                        acc += wy * wx * ((const uchar *)mat_at(src, (int)sy, (int)sx))[c];
                    }
                }
                ((uchar *)mat_at(dst, (int)y, (int)x))[c] = (uchar)((acc + den / 2) / den);
            }
        }
    }
}

static void check_area_against_reference(int type, int srows, int scols,
                                         int drows, int dcols, unsigned seed) {
    struct Mat *src = alloc_mat(srows, scols, type);
    struct Mat *dst = alloc_mat(drows, dcols, type);
    struct Mat *ref = alloc_mat(drows, dcols, type);
    fill_pattern(src, seed);
    resize(dst, src, INTER_AREA);
    area_reference(ref, src);
    CHECK_MAT_BYTES(dst, ref->data);
    free_mat(&ref);
    free_mat(&dst);
    free_mat(&src);
}

static void test_area_width_sweep(void) {
    for (int sw = 1; sw <= 9; sw++) {
        for (int dw = 0; dw <= 65; dw++) {
            check_area_against_reference(CV_8UC1, 1, sw, 1, dw, (unsigned)sw);
        }
    }
    for (int sw = 10; sw <= 65; sw++) {
        for (int dw = 1; dw <= 12; dw++) {
            check_area_against_reference(CV_8UC3, 1, sw, 1, dw, (unsigned)sw);
        }
    }
}

static void test_area_2d_sweep(void) {
    static const int types[] = {CV_8UC1, CV_8UC3, CV_8UC4};
    for (int t = 0; t < 3; t++) {
        for (int sr = 1; sr <= 7; sr++) {
            for (int sc = 1; sc <= 7; sc++) {
                for (int dr = 1; dr <= 9; dr += 2) {
                    for (int dc = 1; dc <= 9; dc += 2) {
                        check_area_against_reference(types[t], sr, sc, dr, dc,
                                                     (unsigned)(sr * 7 + sc));
                    }
                }
            }
        }
    }
    check_area_against_reference(CV_8UC3, 40, 64, 17, 29, 5);
    check_area_against_reference(CV_8UC1, 3, 65, 65, 3, 9);
}

static void test_area_constant_255(void) {
    struct Mat *src = alloc_mat(13, 17, CV_8UC3);
    memset(src->data, 255, (size_t)src->rows * src->step[0]);
    struct Mat *dst = alloc_mat(5, 31, CV_8UC3);
    resize(dst, src, INTER_AREA);
    for (size_t i = 0; i < (size_t)dst->rows * dst->step[0]; i++) {
        CHECK_EQ_INT(dst->data[i], 255);
    }
    free_mat(&dst);
    free_mat(&src);
}

static void test_area_zero_sized_dst(void) {
    struct Mat *src = alloc_mat(3, 3, CV_8UC1);
    fill_pattern(src, 1);
    struct Mat *dst = alloc_mat(0, 5, CV_8UC1);
    resize(dst, src, INTER_AREA);
    free_mat(&dst);
    dst = alloc_mat(4, 0, CV_8UC1);
    resize(dst, src, INTER_AREA);
    free_mat(&dst);
    free_mat(&src);
}

static void test_area_scratch_balanced(void) {
    struct Mat *src = alloc_mat(40, 64, CV_8UC3);
    struct Mat *dst = alloc_mat(17, 29, CV_8UC3);
    fill_pattern(src, 3);
    alloc_counter_start();
    resize(dst, src, INTER_AREA);
    alloc_counter_stop();
    CHECK_EQ_INT(alloc_stats.allocs, 1);
    CHECK_EQ_INT(alloc_stats.frees, 1);
    CHECK_EQ_INT(alloc_stats.current, 0);
    CHECK(alloc_stats.peak >= 2 * 29 * 3 * 4);
    CHECK(alloc_stats.peak <= 2 * 29 * 3 * 4 + 64);
    free_mat(&dst);
    free_mat(&src);
}

static jmp_buf assert_jump;
static int     caught_count;

static void jump_handler(const char *expr, const char *file, int line) {
    (void)expr;
    (void)file;
    (void)line;
    caught_count++;
    longjmp(assert_jump, 1);
}

static void test_area_misuse_asserts(void) {
    // 2^24 + 1 pixels: one more than INTER_AREA accepts.
    struct Mat *big = alloc_mat(1, (1 << 24) + 1, CV_8UC1);
    struct Mat *dst = alloc_mat(1, 4, CV_8UC1);
    struct Mat *rgb = alloc_mat(1, 4, CV_8UC3);
    CHECK(big != NULL && dst != NULL && rgb != NULL);
    if (big == NULL || dst == NULL || rgb == NULL) {
        free_mat(&big);
        free_mat(&dst);
        free_mat(&rgb);
        return;
    }
    caught_count = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        resize(dst, big, INTER_AREA);
        CHECK(0 && "INTER_AREA accepted more than 2^24 source pixels");
    }
    if (setjmp(assert_jump) == 0) {
        resize(rgb, dst, INTER_AREA); // type mismatch
        CHECK(0 && "INTER_AREA accepted mismatched types");
    }
    if (setjmp(assert_jump) == 0) {
        resize(dst, dst, INTER_AREA); // in place
        CHECK(0 && "INTER_AREA accepted src == dst");
    }
    improc_set_assert_handler(NULL);
    CHECK_EQ_INT(caught_count, 3);
    free_mat(&rgb);
    free_mat(&dst);
    free_mat(&big);
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
    RUN_TEST(test_area_weight_tables);
    RUN_TEST(test_area_hand_cases);
    RUN_TEST(test_area_rgb_channels_independent);
    RUN_TEST(test_area_width_sweep);
    RUN_TEST(test_area_2d_sweep);
    RUN_TEST(test_area_constant_255);
    RUN_TEST(test_area_zero_sized_dst);
    RUN_TEST(test_area_scratch_balanced);
    RUN_TEST(test_area_misuse_asserts);
    return test_finish();
}
