#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "improc.h"
#include "improc_internal.h"
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

// ---- Otsu ------------------------------------------------------------------

static uchar otsu_of_bytes(int rows, int cols, const uchar *in) {
    struct Mat *src = mat_from_bytes(rows, cols, CV_8UC1, in);
    uchar       t   = otsu_threshold(src);
    free_mat(&src);
    return t;
}

static void test_otsu_hand(void) {
    const uchar a[] = {0, 1, 2, 3}; // f(0) = 12, f(1) = 13, f(2) = 12
    const uchar b[] = {10, 10, 10, 200, 200, 200};
    const uchar c[] = {1, 2, 3, 4, 5, 6};
    const uchar d[] = {10, 20, 20, 30, 30, 30, 200, 210};
    const uchar e[] = {5, 5, 9, 9};
    const uchar f[] = {0, 255};
    const uchar g[] = {77, 77, 77, 77};
    CHECK_EQ_INT(otsu_of_bytes(1, 4, a), 1);
    CHECK_EQ_INT(otsu_of_bytes(2, 3, b), 10);
    CHECK_EQ_INT(otsu_of_bytes(1, 6, c), 3);
    CHECK_EQ_INT(otsu_of_bytes(2, 4, d), 30);
    CHECK_EQ_INT(otsu_of_bytes(1, 4, e), 5);
    CHECK_EQ_INT(otsu_of_bytes(1, 2, f), 0);
    CHECK_EQ_INT(otsu_of_bytes(1, 4, g), 0);
}

static void test_otsu_degenerate(void) {
    uint32_t hist[256];
    memset(hist, 0, sizeof hist);
    CHECK_EQ_INT(otsu_threshold_hist(hist), 0); // empty
    hist[255] = 9;
    CHECK_EQ_INT(otsu_threshold_hist(hist), 0); // one level, the top one
    hist[0] = 1;
    CHECK_EQ_INT(otsu_threshold_hist(hist), 0); // levels 0 and 255
    memset(hist, 0, sizeof hist);
    hist[254] = 1;
    hist[255] = 1;
    CHECK_EQ_INT(otsu_threshold_hist(hist), 254);
    struct Mat *empty = alloc_mat(0, 4, CV_8UC1);
    CHECK_EQ_INT(otsu_threshold(empty), 0);
    free_mat(&empty);
}

// Exact reference for small totals. f(t) = n / d with n = s0^2 * w1 + s1^2 * w0
// and d = w0 * w1; fractions are compared by cross multiplication. total <= 255
// keeps every product below 2^64.
static uchar ref_otsu_small(const uint32_t hist[256]) {
    uint64_t total = 0, sum = 0;
    for (int i = 0; i < 256; i++) {
        total += hist[i];
        sum += (uint64_t)hist[i] * (uint64_t)i;
    }
    int      found  = 0;
    uchar    best_t = 0;
    uint64_t best_n = 0, best_d = 1;
    for (int t = 0; t < 255; t++) {
        uint64_t w0 = 0, s0 = 0;
        for (int i = 0; i <= t; i++) {
            w0 += hist[i];
            s0 += (uint64_t)hist[i] * (uint64_t)i;
        }
        uint64_t w1 = total - w0;
        if (w0 == 0 || w1 == 0) {
            continue;
        }
        uint64_t s1 = sum - s0;
        uint64_t n  = s0 * s0 * w1 + s1 * s1 * w0;
        uint64_t d  = w0 * w1;
        if (!found || n * best_d > best_n * d) {
            found  = 1;
            best_t = (uchar)t;
            best_n = n;
            best_d = d;
        }
    }
    return best_t;
}

static void test_otsu_random_small_vs_reference(void) {
    for (int iter = 0; iter < 3000; iter++) {
        uint32_t hist[256];
        memset(hist, 0, sizeof hist);
        int pixels = 1 + (int)(lcg_u8() % 250);
        // A narrow, shifting value range makes exact ties common.
        int range = 1 + lcg_u8() % 55;
        int base  = lcg_u8() % 200;
        for (int p = 0; p < pixels; p++) {
            hist[base + lcg_u8() % range]++;
        }
        CHECK_EQ_INT(otsu_threshold_hist(hist), ref_otsu_small(hist));
    }
}

#ifdef __SIZEOF_INT128__
// Host-only reference for larger counts: with a total of at most 2^16,
// s0^2 * w1 + s1^2 * w0 < 2^66, d < 2^32 and the cross product < 2^98.
static uchar ref_otsu_mid(const uint32_t hist[256]) {
    unsigned __int128 total = 0, sum = 0;
    for (int i = 0; i < 256; i++) {
        total += hist[i];
        sum += (unsigned __int128)hist[i] * (unsigned)i;
    }
    int               found  = 0;
    uchar             best_t = 0;
    unsigned __int128 best_n = 0, best_d = 1;
    unsigned __int128 w0 = 0, s0 = 0;
    for (int t = 0; t < 255; t++) {
        w0 += hist[t];
        s0 += (unsigned __int128)hist[t] * (unsigned)t;
        unsigned __int128 w1 = total - w0;
        if (w0 == 0 || w1 == 0) {
            continue;
        }
        unsigned __int128 s1 = sum - s0;
        unsigned __int128 n  = s0 * s0 * w1 + s1 * s1 * w0;
        unsigned __int128 d  = w0 * w1;
        if (!found || n * best_d > best_n * d) {
            found  = 1;
            best_t = (uchar)t;
            best_n = n;
            best_d = d;
        }
    }
    return best_t;
}

static void test_otsu_random_mid_vs_int128(void) {
    for (int iter = 0; iter < 1000; iter++) {
        uint32_t hist[256];
        memset(hist, 0, sizeof hist);
        int range = 1 + lcg_u8() % 255;
        int base  = lcg_u8() % (256 - range + 1);
        for (int p = 0; p < 20; p++) {
            lcg_state    = lcg_state * 1664525u + 1013904223u;
            uint32_t cnt = (lcg_state >> 8) % 3000u;
            hist[base + lcg_u8() % range] += cnt;
        }
        CHECK_EQ_INT(otsu_threshold_hist(hist), ref_otsu_mid(hist));
    }
}
#endif

static void test_otsu_overflow_probes(void) {
    uint32_t hist[256];
    memset(hist, 0, sizeof hist);
    hist[0]   = 1u << 23;
    hist[255] = 1u << 23;
    CHECK_EQ_INT(otsu_threshold_hist(hist), 0);

    const int ks[] = {0, 1, 100, 254};
    for (int i = 0; i < 4; i++) {
        memset(hist, 0, sizeof hist);
        hist[255] = (1u << 24) - 1u;
        hist[ks[i]] += 1;
        CHECK_EQ_INT(otsu_threshold_hist(hist), ks[i]);
    }

    // Three levels with counts near 2^24 in total: the plateau of the best
    // split starts at its lowest populated level.
    memset(hist, 0, sizeof hist);
    hist[10]  = 9000000u;
    hist[200] = 7000000u;
    hist[255] = 700000u;
    CHECK_EQ_INT(otsu_threshold_hist(hist), 10);

    // Exactly 2^24 pixels is accepted, one more is rejected.
    memset(hist, 0, sizeof hist);
    hist[3] = 1u << 24;
    CHECK_EQ_INT(otsu_threshold_hist(hist), 0);
    hist[4] = 1;
    CHECK_ASSERTS(otsu_threshold_hist(hist));
}

static void test_u128_multiply(void) {
    struct improc_u128 r = improc_mul_u64(UINT64_MAX, UINT64_MAX);
    CHECK(r.hi == UINT64_MAX - 1u);
    CHECK(r.lo == 1u);
    r = improc_mul_u64(0, UINT64_MAX);
    CHECK(r.hi == 0 && r.lo == 0);
    r = improc_mul_u64(1ull << 31, 1ull << 31); // 2^62
    CHECK(r.hi == 0 && r.lo == (1ull << 62));
    r = improc_mul_u64(1ull << 32, 1ull << 32); // 2^64
    CHECK(r.hi == 1 && r.lo == 0);
    CHECK(improc_u128_greater(improc_mul_u64(1ull << 40, 1ull << 40),
                              improc_mul_u64(UINT64_MAX, 1ull << 15)));
    CHECK(!improc_u128_greater(improc_mul_u64(5, 7), improc_mul_u64(35, 1)));
    CHECK(!improc_u128_greater(improc_mul_u64(5, 6), improc_mul_u64(35, 1)));
#ifdef __SIZEOF_INT128__
    const uint64_t v[] = {1,
                          2,
                          0xFFFFFFFFull,
                          0x100000000ull,
                          0xFFFFFFFFFFFFull,
                          0x8000000000000001ull,
                          0x123456789ABCDEF0ull,
                          UINT64_MAX - 3};
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            unsigned __int128  want = (unsigned __int128)v[i] * v[j];
            struct improc_u128 got  = improc_mul_u64(v[i], v[j]);
            CHECK(got.hi == (uint64_t)(want >> 64) && got.lo == (uint64_t)want);
        }
    }
#endif
}

static void test_otsu_width_sweep(void) {
    for (int w = 0; w <= 65; w++) {
        struct Mat *src = alloc_mat(3, w, CV_8UC1);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < w; x++) {
                // Two populations around 60 and 180.
                uchar base         = (lcg_u8() & 1) ? 180 : 60;
                mat_row(src, y)[x] = (uchar)(base + lcg_u8() % 20);
            }
        }
        uint32_t hist[256];
        calc_hist(hist, src);
        uchar t = otsu_threshold(src);
        CHECK_EQ_INT(t, otsu_threshold_hist(hist));
        CHECK_EQ_INT(t, ref_otsu_small(hist)); // 3 * 65 = 195 pixels at most
        free_mat(&src);
    }
}

static void test_otsu_binarize(void) {
    const uchar in[]  = {10, 20, 20, 30, 30, 30, 200, 210};
    const uchar out[] = {0, 0, 0, 0, 0, 0, 255, 255};
    struct Mat *src   = mat_from_bytes(2, 4, CV_8UC1, in);
    struct Mat *dst   = alloc_mat(2, 4, CV_8UC1);
    threshold(dst, src, otsu_threshold(src), 255, THRESH_BINARY);
    CHECK_MAT_BYTES(dst, out);
    free_mat(&src);
    free_mat(&dst);
}

static void test_otsu_misuse_and_scratch(void) {
    struct Mat *c3 = alloc_mat(2, 2, CV_8UC3);
    CHECK_ASSERTS(otsu_threshold(c3));
    CHECK_ASSERTS(otsu_threshold(NULL));
    CHECK_ASSERTS(otsu_threshold_hist(NULL));
    free_mat(&c3);

    struct Mat *src = alloc_mat(6, 9, CV_8UC1);
    fill_pattern(src, 3);
    alloc_counter_start();
    (void)otsu_threshold(src);
    alloc_counter_stop();
    CHECK_EQ_INT(alloc_stats.allocs, 1);
    CHECK_EQ_INT(alloc_stats.frees, 1);
    CHECK(alloc_stats.peak >= HIST_OTSU_SCRATCH &&
          alloc_stats.peak <= HIST_OTSU_SCRATCH + 32); // + alignment

    improc_set_allocator(null_alloc, null_free);
    CHECK_ASSERTS((void)otsu_threshold(src));
    improc_set_allocator(NULL, NULL);
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
    RUN_TEST(test_otsu_hand);
    RUN_TEST(test_otsu_degenerate);
    RUN_TEST(test_otsu_random_small_vs_reference);
#ifdef __SIZEOF_INT128__
    RUN_TEST(test_otsu_random_mid_vs_int128);
#endif
    RUN_TEST(test_otsu_overflow_probes);
    RUN_TEST(test_u128_multiply);
    RUN_TEST(test_otsu_width_sweep);
    RUN_TEST(test_otsu_binarize);
    RUN_TEST(test_otsu_misuse_and_scratch);
    return test_finish();
}
