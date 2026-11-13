#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>

#include "improc.h"
#include "improc_internal.h"
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

// ---- Allocator that fails the Nth request ----------------------------------

static long alloc_calls;
static long fail_at; // 1-based index of the failing request; 0 = never
static long live_blocks;

static void *nth_alloc(size_t size) {
    alloc_calls++;
    if (fail_at != 0 && alloc_calls == fail_at) {
        return NULL;
    }
    live_blocks++;
    return malloc(size);
}

static void nth_free(void *ptr) {
    live_blocks--;
    free(ptr);
}

static void nth_start(long fail_index) {
    alloc_calls = 0;
    fail_at     = fail_index;
    live_blocks = 0;
    improc_set_allocator(nth_alloc, nth_free);
}

static void nth_stop(void) { improc_set_allocator(NULL, NULL); }

// Runs stmt once to count its allocations, then once per allocation with that
// allocation failing. Each failing run must ASSERT and leave no live block.
#define CHECK_ALLOC_FAILURES(stmt, expected_allocs)                            \
    do {                                                                       \
        nth_start(0);                                                          \
        stmt;                                                                  \
        long total_ = alloc_calls;                                             \
        CHECK_EQ_INT(live_blocks, 0);                                          \
        CHECK_EQ_INT(total_, expected_allocs);                                 \
        for (long k_ = 1; k_ <= total_; k_++) {                                \
            nth_start(k_);                                                     \
            CHECK_ASSERTS(stmt);                                               \
            CHECK_EQ_INT(live_blocks, 0);                                      \
        }                                                                      \
        nth_stop();                                                            \
    } while (0)

// ---- Checked size helpers --------------------------------------------------

static void test_mul_size(void) {
    size_t out = 0;
    CHECK(improc_mul_size(0, SIZE_MAX, &out) && out == 0);
    CHECK(improc_mul_size(SIZE_MAX, 1, &out) && out == SIZE_MAX);
    CHECK(improc_mul_size(1, SIZE_MAX, &out) && out == SIZE_MAX);
    CHECK(improc_mul_size(12, 34, &out) && out == 408);

    // The largest exact product, and one more than it.
    size_t half = SIZE_MAX / 2;
    CHECK(improc_mul_size(half, 2, &out) && out == half * 2);
    out = 77;
    CHECK(!improc_mul_size(half + 1, 2, &out));
    CHECK_EQ_INT(out, 77); // untouched on failure
    CHECK(!improc_mul_size(SIZE_MAX, 2, &out));
    CHECK(!improc_mul_size(SIZE_MAX, SIZE_MAX, &out));

    // A power of two times itself wraps to 0 without a check. This is the
    // 16-bit AVR case (256 * 256) in the width of this size_t.
    size_t sq = (size_t)1 << (sizeof(size_t) * CHAR_BIT / 2);
    CHECK(!improc_mul_size(sq, sq, &out));
    CHECK(improc_mul_size(sq - 1, sq + 1, &out) && out == SIZE_MAX);
}

static void test_add_size(void) {
    size_t out = 0;
    CHECK(improc_add_size(0, 0, &out) && out == 0);
    CHECK(improc_add_size(SIZE_MAX, 0, &out) && out == SIZE_MAX);
    CHECK(improc_add_size(SIZE_MAX - 5, 5, &out) && out == SIZE_MAX);
    out = 77;
    CHECK(!improc_add_size(SIZE_MAX, 1, &out));
    CHECK_EQ_INT(out, 77);
    CHECK(!improc_add_size(SIZE_MAX, SIZE_MAX, &out));
}

static void test_scratch_forms_assert(void) {
    CHECK_EQ_INT(improc_scratch_mul(6, 7), 42);
    CHECK_EQ_INT(improc_scratch_add(6, 7), 13);
    CHECK_ASSERTS(improc_scratch_mul(SIZE_MAX, 2));
    CHECK_ASSERTS(improc_scratch_mul(SIZE_MAX / 2 + 1, 2));
    CHECK_ASSERTS(improc_scratch_add(SIZE_MAX, 1));
}

#if SIZE_MAX <= 0xFFFFFFFFu
// With a 32-bit size_t a Mat header with huge dimensions makes the ring size
// wrap: 3 rows * (INT32_MAX / 4 cols * 4 channels) * 2 bytes is about 2^34.
// The ops must ASSERT before they allocate. This is not reachable with a
// 64-bit size_t, so this test only exists on 32-bit and 16-bit targets. The
// header points at a tiny buffer that nothing reads.
static void test_wrap_is_caught(void) {
    static uchar buf[16];
    struct Mat   big;
    big.flags        = (uint32_t)CV_8UC4 | (uint32_t)MAGIC_VAL;
    big.dims         = 2;
    big.rows         = 3;
    big.cols         = (int)(INT32_MAX / 4);
    big.size[0]      = 3;
    big.size[1]      = (size_t)big.cols;
    big.step[0]      = (size_t)big.cols * 4;
    big.step[1]      = 4;
    big.data         = buf;
    struct Mat other = big;
    other.data       = buf + 8;
    nth_start(0);
    CHECK_ASSERTS(box_filter(&other, &big, 3));
    CHECK_ASSERTS(gaussian_blur(&other, &big, 3));
    CHECK_EQ_INT(alloc_calls, 0); // failed before the first allocation
    nth_stop();
}
#endif

// ---- Allocation failure is an ASSERT, with no leak -------------------------

static void test_alloc_failure_filter(void) {
    struct Mat *src = alloc_mat(9, 11, CV_8UC3);
    struct Mat *dst = alloc_mat(9, 11, CV_8UC3);
    struct Mat *s1  = alloc_mat(9, 11, CV_8UC1);
    struct Mat *d16 = alloc_mat(9, 11, CV_16SC1);
    fill_pattern(src, 1);
    fill_pattern(s1, 2);
    CHECK_ALLOC_FAILURES(box_filter(dst, src, 5), 3);
    CHECK_ALLOC_FAILURES(gaussian_blur(dst, src, 5), 2);
    CHECK_ALLOC_FAILURES(sobel(d16, s1, 1, 0), 2);
    free_mat(&src);
    free_mat(&dst);
    free_mat(&s1);
    free_mat(&d16);
}

static void test_alloc_failure_morph(void) {
    struct Mat *src = alloc_mat(9, 11, CV_8UC3);
    struct Mat *dst = alloc_mat(9, 11, CV_8UC3);
    struct Mat *tmp = alloc_mat(9, 11, CV_8UC3);
    fill_pattern(src, 3);
    CHECK_ALLOC_FAILURES(erode(dst, src, 3), 1);
    CHECK_ALLOC_FAILURES(dilate(dst, src, 3), 1);
    CHECK_ALLOC_FAILURES(morph_open(dst, tmp, src, 3), 2);
    free_mat(&src);
    free_mat(&dst);
    free_mat(&tmp);
}

static void test_alloc_failure_geometry(void) {
    struct Mat *src = alloc_mat(9, 11, CV_8UC3);
    struct Mat *dst = alloc_mat(5, 7, CV_8UC3);
    fill_pattern(src, 4);
    CHECK_ALLOC_FAILURES(resize(dst, src, INTER_NEAREST), 2);
    CHECK_ALLOC_FAILURES(resize(dst, src, INTER_LINEAR), 2);
    CHECK_ALLOC_FAILURES(resize(dst, src, INTER_AREA), 1);
    free_mat(&src);
    free_mat(&dst);
}

static void test_alloc_failure_hist(void) {
    struct Mat *src = alloc_mat(9, 11, CV_8UC1);
    struct Mat *dst = alloc_mat(9, 11, CV_8UC1);
    fill_pattern(src, 5);
    CHECK_ALLOC_FAILURES(equalize_hist(dst, src), 1);
    CHECK_ALLOC_FAILURES((void)otsu_threshold(src), 1);
    free_mat(&src);
    free_mat(&dst);
}

// calc_hist and otsu_threshold_hist take a caller-owned histogram and need no
// scratch, so they work with an allocator that always fails.
static void *null_alloc(size_t size) {
    (void)size;
    return NULL;
}

static void null_free(void *ptr) { (void)ptr; }

static void test_hist_needs_no_scratch(void) {
    struct Mat *src = alloc_mat(9, 11, CV_8UC1);
    fill_pattern(src, 6);
    uint32_t want[256];
    calc_hist(want, src);
    uchar want_t = otsu_threshold_hist(want);

    improc_set_allocator(null_alloc, null_free);
    uint32_t got[256];
    calc_hist(got, src);
    uchar got_t = otsu_threshold_hist(got);
    improc_set_allocator(NULL, NULL);

    CHECK_EQ_BYTES(got, want, sizeof want);
    CHECK_EQ_INT(got_t, want_t);
    free_mat(&src);
}

int main(void) {
    RUN_TEST(test_mul_size);
    RUN_TEST(test_add_size);
    RUN_TEST(test_scratch_forms_assert);
#if SIZE_MAX <= 0xFFFFFFFFu
    RUN_TEST(test_wrap_is_caught);
#endif
    RUN_TEST(test_alloc_failure_filter);
    RUN_TEST(test_alloc_failure_morph);
    RUN_TEST(test_alloc_failure_geometry);
    RUN_TEST(test_alloc_failure_hist);
    RUN_TEST(test_hist_needs_no_scratch);
    return test_finish();
}
