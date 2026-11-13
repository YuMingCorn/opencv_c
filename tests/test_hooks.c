#include <setjmp.h>
#include <stdint.h>

#include "improc.h"
#include "test_alloc_util.h"
#include "test_mat_util.h"
#include "test_util.h"

static jmp_buf     assert_jump;
static const char *caught_expr;
static int         caught_line;
static int         caught_count;

static void jump_handler(const char *expr, const char *file, int line) {
    (void)file;
    caught_expr = expr;
    caught_line = line;
    caught_count++;
    longjmp(assert_jump, 1);
}

static void test_assert_handler_catches_misuse(void) {
    struct Mat *src = alloc_mat(3, 3, CV_8UC1);
    struct Mat *dst = alloc_mat(2, 3, CV_8UC1);
    caught_expr     = NULL;
    caught_line     = 0;
    caught_count    = 0;

    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        threshold(dst, src, 128, 255, THRESH_BINARY);
        CHECK(0 && "threshold accepted a mismatched dst");
    }
    improc_set_assert_handler(NULL);

    CHECK_EQ_INT(caught_count, 1);
    CHECK(caught_line > 0);
#ifdef IMPROC_ASSERT_TERSE
    CHECK(caught_expr == NULL);
#else
    CHECK(caught_expr != NULL);
#endif
    free_mat(&dst);
    free_mat(&src);
}

static void test_assert_handler_direct_call(void) {
    caught_count = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        improc_assert_fail("x > 0", "file.c", 42);
    }
    improc_set_assert_handler(NULL);
    CHECK_EQ_INT(caught_count, 1);
    CHECK_EQ_INT(caught_line, 42);
    CHECK(caught_expr != NULL && caught_expr[0] == 'x');
}

static void test_allocator_hook_balanced_for_box_filter(void) {
    struct Mat *src = alloc_mat(8, 9, CV_8UC3);
    struct Mat *dst = alloc_mat(8, 9, CV_8UC3);
    fill_pattern(src, 3);

    alloc_counter_start();
    box_filter(dst, src, 5);
    alloc_counter_stop();

    CHECK(alloc_stats.allocs > 0);
    CHECK_EQ_INT(alloc_stats.allocs, alloc_stats.frees);
    CHECK_EQ_INT(alloc_stats.current, 0);
    CHECK(alloc_stats.peak > 0);
    free_mat(&dst);
    free_mat(&src);
}

static void test_alloc_mat_uses_hook(void) {
    alloc_counter_start();
    struct Mat *m      = alloc_mat(4, 4, CV_8UC1);
    long        during = alloc_stats.allocs;
    free_mat(&m);
    alloc_counter_stop();

    CHECK(during > 0);
    CHECK_EQ_INT(alloc_stats.allocs, alloc_stats.frees);
    CHECK_EQ_INT(alloc_stats.current, 0);
}

static unsigned char odd_pool[4096];
static size_t        odd_used;

static void *odd_alloc(size_t size) {
    // Hands out odd addresses, so alignment is left to fast_malloc.
    size_t start = odd_used | 1u;
    if (start + size > sizeof(odd_pool)) {
        return NULL;
    }
    odd_used = start + size + 1;
    return odd_pool + start;
}

static void odd_free(void *ptr) { (void)ptr; }

static void test_fast_malloc_aligns_odd_allocator(void) {
    odd_used = 0;
    improc_set_allocator(odd_alloc, odd_free);
    for (size_t size = 0; size < 40; size += 7) {
        void *p = fast_malloc(size);
        CHECK(p != NULL);
        if (p != NULL) {
            CHECK_EQ_INT((uintptr_t)p % 16, 0);
        }
        fast_free(p);
    }
    improc_set_allocator(NULL, NULL);
}

static void *failing_alloc(size_t size) {
    (void)size;
    return NULL;
}

static void test_failing_allocator(void) {
    improc_set_allocator(failing_alloc, odd_free);
    CHECK(fast_malloc(16) == NULL);
    CHECK(mat_create(2, 2, CV_8UC1) == NULL);
    improc_set_allocator(NULL, NULL);

    void *p = fast_malloc(16);
    CHECK(p != NULL);
    fast_free(p);
}

int main(void) {
    RUN_TEST(test_assert_handler_catches_misuse);
    RUN_TEST(test_assert_handler_direct_call);
    RUN_TEST(test_allocator_hook_balanced_for_box_filter);
    RUN_TEST(test_alloc_mat_uses_hook);
    RUN_TEST(test_fast_malloc_aligns_odd_allocator);
    RUN_TEST(test_failing_allocator);
    return test_finish();
}
