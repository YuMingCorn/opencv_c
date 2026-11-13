#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "improc.h"
#include "test_alloc_util.h"
#include "test_mat_util.h"

static jmp_buf assert_jump;
static int     caught_count;

static void jump_handler(const char *expr, const char *file, int line) {
    (void)expr;
    (void)file;
    (void)line;
    caught_count++;
    longjmp(assert_jump, 1);
}

// Runs stmt with the longjmp handler installed and checks that it ASSERTs.
#define CHECK_ASSERTS(stmt)                                                    \
    do {                                                                       \
        caught_count = 0;                                                      \
        improc_set_assert_handler(jump_handler);                               \
        if (setjmp(assert_jump) == 0) {                                        \
            stmt;                                                              \
        }                                                                      \
        improc_set_assert_handler(NULL);                                       \
        CHECK_EQ_INT(caught_count, 1);                                         \
    } while (0)

static uint32_t lcg_state = 12345u;

static uchar lcg_u8(void) {
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return (uchar)(lcg_state >> 24);
}

// ---- calc_hist -------------------------------------------------------------

static void test_calc_hist_hand(void) {
    const uchar in[] = {0, 0, 1, 1, 2, 2, 3, 3};
    struct Mat *src  = mat_from_bytes(2, 4, CV_8UC1, in);
    uint32_t    hist[256];
    memset(hist, 0xFF, sizeof hist);
    calc_hist(hist, src);
    for (int i = 0; i < 256; i++) {
        CHECK_EQ_INT(hist[i], i < 4 ? 2 : 0);
    }
    free_mat(&src);
}

static void test_calc_hist_extremes(void) {
    const uchar in[] = {255, 255, 255, 0, 7};
    struct Mat *src  = mat_from_bytes(1, 5, CV_8UC1, in);
    uint32_t    hist[256];
    calc_hist(hist, src);
    CHECK_EQ_INT(hist[255], 3);
    CHECK_EQ_INT(hist[0], 1);
    CHECK_EQ_INT(hist[7], 1);
    CHECK_EQ_INT(hist[1], 0);
    free_mat(&src);
}

static void test_calc_hist_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src = alloc_mat(3, w, CV_8UC1);
        uint32_t    ref[256];
        uint32_t    hist[256];
        memset(ref, 0, sizeof ref);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                uchar v            = lcg_u8();
                mat_row(src, y)[x] = v;
                ref[v]++;
            }
        }
        calc_hist(hist, src);
        CHECK_EQ_BYTES(hist, ref, sizeof ref);
        free_mat(&src);
    }
}

static void test_calc_hist_zero_size(void) {
    // An empty image has an all-zero histogram; the output is cleared.
    const int shapes[3][2] = {{0, 0}, {0, 5}, {4, 0}};
    for (int k = 0; k < 3; k++) {
        struct Mat *src = alloc_mat(shapes[k][0], shapes[k][1], CV_8UC1);
        uint32_t    hist[256];
        memset(hist, 0x55, sizeof hist);
        calc_hist(hist, src);
        for (int i = 0; i < 256; i++) {
            CHECK_EQ_INT(hist[i], 0);
        }
        free_mat(&src);
    }
}

static void test_calc_hist_pixel_limit(void) {
    struct Mat *big = alloc_mat(1, 16777216, CV_8UC1);
    if (big == NULL) {
        TEST_SKIP("cannot allocate 16 MiB");
        return;
    }
    memset(big->data, 9, 16777216);
    uint32_t hist[256];
    calc_hist(hist, big);
    CHECK_EQ_INT(hist[9], 16777216);
    CHECK_EQ_INT(hist[0], 0);
    free_mat(&big);

    struct Mat *over = alloc_mat(1, 16777217, CV_8UC1);
    if (over != NULL) {
        CHECK_ASSERTS(calc_hist(hist, over));
        free_mat(&over);
    }
}

static void test_calc_hist_misuse(void) {
    uint32_t    hist[256];
    struct Mat *c3 = alloc_mat(2, 2, CV_8UC3);
    struct Mat *g  = alloc_mat(2, 2, CV_8UC1);
    CHECK_ASSERTS(calc_hist(hist, c3));
    CHECK_ASSERTS(calc_hist(NULL, g));
    CHECK_ASSERTS(calc_hist(hist, NULL));
    free_mat(&c3);
    free_mat(&g);
}

static void test_calc_hist_no_alloc(void) {
    struct Mat *src = alloc_mat(5, 7, CV_8UC1);
    fill_pattern(src, 1);
    uint32_t hist[256];
    alloc_counter_start();
    calc_hist(hist, src);
    alloc_counter_stop();
    CHECK_EQ_INT(alloc_stats.allocs, 0);
    free_mat(&src);
}

// ---- equalize_hist ---------------------------------------------------------

static void check_equalize(int rows, int cols, const uchar *in,
                           const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, CV_8UC1, in);
    struct Mat *dst = alloc_mat(rows, cols, CV_8UC1);
    equalize_hist(dst, src);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

static void test_equalize_hand_uniform(void) {
    const uchar in[]  = {0, 0, 1, 1, 2, 2, 3, 3};
    const uchar out[] = {0, 0, 85, 85, 170, 170, 255, 255};
    check_equalize(2, 4, in, out);
}

static void test_equalize_hand_offset_levels(void) {
    // m = 10, den = 5: lut[50] = (2 * 255 + 2) / 5, lut[200] = 255.
    const uchar in[]  = {10, 10, 10, 50, 50, 200, 200, 200};
    const uchar out[] = {0, 0, 0, 102, 102, 255, 255, 255};
    check_equalize(2, 4, in, out);
}

static void test_equalize_hand_ramp(void) {
    // lut[1] = (1 * 255 + 1) / 2 = 128, rounded half up.
    const uchar in[]  = {0, 1, 2};
    const uchar out[] = {0, 128, 255};
    check_equalize(1, 3, in, out);
}

static void test_equalize_constant_is_copy(void) {
    const uchar levels[] = {0, 7, 255};
    for (int k = 0; k < 3; k++) {
        uchar in[6], out[6];
        memset(in, levels[k], sizeof in);
        memset(out, levels[k], sizeof out);
        check_equalize(2, 3, in, out);
    }
}

static void test_equalize_single_pixel(void) {
    const uchar in[] = {99};
    check_equalize(1, 1, in, in);
}

static uchar ref_equalize_level(const uint32_t hist[256], uint64_t total,
                                int v) {
    int m = 0;
    while (hist[m] == 0) {
        m++;
    }
    uint64_t den = total - hist[m];
    if (den == 0) {
        return (uchar)v;
    }
    if (v <= m) {
        return 0;
    }
    uint64_t sum = 0;
    for (int i = m + 1; i <= v; i++) {
        sum += hist[i];
    }
    return (uchar)((sum * 255u + den / 2u) / den);
}

static void test_equalize_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        // A narrow value range makes ties and empty levels likely.
        struct Mat *src = alloc_mat(3, w, CV_8UC1);
        struct Mat *dst = alloc_mat(3, w, CV_8UC1);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                mat_row(src, y)[x] = (uchar)(40 + (lcg_u8() % 90));
            }
        }
        uint32_t hist[256];
        calc_hist(hist, src);
        equalize_hist(dst, src);
        if (w > 0) {
            for (int y = 0; y < 3; y++) {
                for (int x = 0; x < w; x++) {
                    uchar v = mat_row(src, y)[x];
                    CHECK_EQ_INT(mat_row(dst, y)[x],
                                 ref_equalize_level(hist, 3u * (unsigned)w, v));
                }
            }
        }
        free_mat(&src);
        free_mat(&dst);
    }
}

static void test_equalize_properties(void) {
    // Monotone, min level -> 0, max level -> 255 on random full-range data.
    struct Mat *src = alloc_mat(37, 41, CV_8UC1);
    struct Mat *dst = alloc_mat(37, 41, CV_8UC1);
    fill_pattern(src, 5);
    equalize_hist(dst, src);
    int map[256];
    for (int i = 0; i < 256; i++) {
        map[i] = -1;
    }
    int lo = 255, hi = 0;
    for (int y = 0; y < 37; y++) {
        for (int x = 0; x < 41; x++) {
            int s = mat_row(src, y)[x];
            int d = mat_row(dst, y)[x];
            CHECK(map[s] == -1 || map[s] == d);
            map[s] = d;
            lo     = s < lo ? s : lo;
            hi     = s > hi ? s : hi;
        }
    }
    int prev = 0;
    for (int i = 0; i < 256; i++) {
        if (map[i] >= 0) {
            CHECK(map[i] >= prev);
            prev = map[i];
        }
    }
    CHECK_EQ_INT(map[lo], 0);
    CHECK_EQ_INT(map[hi], 255);
    free_mat(&src);
    free_mat(&dst);
}

static void test_equalize_large_counts(void) {
    // 2^24 pixels: sum * 255 stays below 2^32 (sum <= 2^24 - 1).
    struct Mat *src = alloc_mat(1, 16777216, CV_8UC1);
    struct Mat *dst = alloc_mat(1, 16777216, CV_8UC1);
    memset(src->data, 1, 16777216);
    src->data[0] = 0;
    equalize_hist(dst, src);
    CHECK_EQ_INT(dst->data[0], 0);
    CHECK_EQ_INT(dst->data[1], 255);
    CHECK_EQ_INT(dst->data[16777215], 255);
    // Half and half: den = 2^23, lut[200] = (2^23 * 255 + 2^22) / 2^23 = 255.
    memset(src->data, 200, 16777216);
    memset(src->data, 3, 8388608);
    equalize_hist(dst, src);
    CHECK_EQ_INT(dst->data[0], 0);
    CHECK_EQ_INT(dst->data[16777215], 255);
    free_mat(&src);
    free_mat(&dst);
}

static void test_equalize_zero_size(void) {
    const int shapes[3][2] = {{0, 0}, {0, 5}, {4, 0}};
    for (int k = 0; k < 3; k++) {
        struct Mat *src = alloc_mat(shapes[k][0], shapes[k][1], CV_8UC1);
        struct Mat *dst = alloc_mat(shapes[k][0], shapes[k][1], CV_8UC1);
        equalize_hist(dst, src);
        free_mat(&src);
        free_mat(&dst);
    }
}

static void test_equalize_misuse(void) {
    struct Mat *g   = alloc_mat(3, 3, CV_8UC1);
    struct Mat *g2  = alloc_mat(3, 4, CV_8UC1);
    struct Mat *c3  = alloc_mat(3, 3, CV_8UC3);
    struct Mat *g16 = alloc_mat(3, 3, CV_16SC1);
    CHECK_ASSERTS(equalize_hist(g2, g));
    CHECK_ASSERTS(equalize_hist(c3, g));
    CHECK_ASSERTS(equalize_hist(g, c3));
    CHECK_ASSERTS(equalize_hist(g16, g));
    CHECK_ASSERTS(equalize_hist(g, g));
    CHECK_ASSERTS(equalize_hist(NULL, g));
    free_mat(&g);
    free_mat(&g2);
    free_mat(&c3);
    free_mat(&g16);
}

static void *null_alloc(size_t size) {
    (void)size;
    return NULL;
}

static void null_free(void *ptr) { (void)ptr; }

static void test_equalize_scratch(void) {
    struct Mat *src = alloc_mat(5, 7, CV_8UC1);
    struct Mat *dst = alloc_mat(5, 7, CV_8UC1);
    fill_pattern(src, 2);
    alloc_counter_start();
    equalize_hist(dst, src);
    alloc_counter_stop();
    CHECK_EQ_INT(alloc_stats.allocs, 1);
    CHECK_EQ_INT(alloc_stats.frees, 1);
    CHECK_EQ_INT(alloc_stats.current, 0);
    CHECK(alloc_stats.peak >= HIST_EQUALIZE_SCRATCH &&
          alloc_stats.peak <= HIST_EQUALIZE_SCRATCH + 32); // + alignment

    // A NULL block from the allocator fails ASSERT. Nothing leaks.
    improc_set_allocator(null_alloc, null_free);
    CHECK_ASSERTS(equalize_hist(dst, src));
    improc_set_allocator(NULL, NULL);

    // Zero-sized input takes no scratch.
    struct Mat *empty = alloc_mat(0, 4, CV_8UC1);
    struct Mat *edst  = alloc_mat(0, 4, CV_8UC1);
    alloc_counter_start();
    equalize_hist(edst, empty);
    alloc_counter_stop();
    CHECK_EQ_INT(alloc_stats.allocs, 0);
    free_mat(&empty);
    free_mat(&edst);
    free_mat(&src);
    free_mat(&dst);
}

int main(void) {
    RUN_TEST(test_calc_hist_hand);
    RUN_TEST(test_calc_hist_extremes);
    RUN_TEST(test_calc_hist_width_sweep);
    RUN_TEST(test_calc_hist_zero_size);
    RUN_TEST(test_calc_hist_pixel_limit);
    RUN_TEST(test_calc_hist_misuse);
    RUN_TEST(test_calc_hist_no_alloc);
    RUN_TEST(test_equalize_hand_uniform);
    RUN_TEST(test_equalize_hand_offset_levels);
    RUN_TEST(test_equalize_hand_ramp);
    RUN_TEST(test_equalize_constant_is_copy);
    RUN_TEST(test_equalize_single_pixel);
    RUN_TEST(test_equalize_width_sweep);
    RUN_TEST(test_equalize_properties);
    RUN_TEST(test_equalize_large_counts);
    RUN_TEST(test_equalize_zero_size);
    RUN_TEST(test_equalize_misuse);
    RUN_TEST(test_equalize_scratch);
    return test_finish();
}
