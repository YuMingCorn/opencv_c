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

int main(void) {
    RUN_TEST(test_calc_hist_hand);
    RUN_TEST(test_calc_hist_extremes);
    RUN_TEST(test_calc_hist_width_sweep);
    RUN_TEST(test_calc_hist_zero_size);
    RUN_TEST(test_calc_hist_pixel_limit);
    RUN_TEST(test_calc_hist_misuse);
    RUN_TEST(test_calc_hist_no_alloc);
    return test_finish();
}
