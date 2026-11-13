#include <limits.h>
#include <setjmp.h>
#include <stdint.h>

#include "color.h"
#include "mat.h"
#include "test_alloc_util.h"
#include "test_util.h"
#include "utils.h"

static void test_type_macros(void) {
    CHECK_EQ_INT(CV_8UC1, 0);
    CHECK_EQ_INT(CV_8UC3, 16);
    CHECK_EQ_INT(CV_8UC4, 24);

    CHECK_EQ_INT(CV_MAT_DEPTH(CV_32FC3), CV_32F);
    CHECK_EQ_INT(CV_MAT_CN(CV_32FC3), 3);

    CHECK_EQ_INT(CV_CH_SIZE(CV_8U), 1);
    CHECK_EQ_INT(CV_CH_SIZE(CV_16S), 2);
    CHECK_EQ_INT(CV_CH_SIZE(CV_32F), 4);
    CHECK_EQ_INT(CV_CH_SIZE(CV_64F), 8);

    CHECK_EQ_INT(CV_PIXEL_SIZE(CV_8UC4), 4);
    CHECK_EQ_INT(CV_PIXEL_SIZE(CV_32FC3), 12);
}

static void test_alloc_layout(void) {
    struct Mat *m = alloc_mat(2, 3, CV_8UC3);

    CHECK_EQ_INT(m->rows, 2);
    CHECK_EQ_INT(m->cols, 3);
    CHECK_EQ_INT(m->dims, 2);
    CHECK_EQ_INT(m->size[0], 2);
    CHECK_EQ_INT(m->size[1], 3);
    CHECK_EQ_INT(m->step[0], 9);
    CHECK_EQ_INT(m->step[1], 3);
    CHECK_EQ_INT(mat_type(m), CV_8UC3);
    CHECK_EQ_INT(mat_channel_num(m), 3);
    CHECK_EQ_INT(mat_pixel_size(m), 3);

    free_mat(&m);
    CHECK(m == NULL);
}

static void test_mat_at_is_interleaved(void) {
    struct Mat *m = alloc_mat(2, 3, CV_8UC3);

    // row 1 starts at byte 9; column 2 adds 2 * 3 bytes.
    CHECK((uchar *)mat_at(m, 1, 2) == m->data + 15);
    CHECK(MAT_AT(*m, 1, 2) == m->data + 15);

    free_mat(&m);
}

static void test_synthetic_pattern(void) {
    struct Mat *m = alloc_mat_synthetic(3, 3);

    // clang-format off
    static const uchar expected[27] = {
        /* row 0 */   0,   0,   0,   127,   0,  63,   255,   0, 127,
        /* row 1 */   0, 127,  63,   127, 127, 127,   255, 127, 191,
        /* row 2 */   0, 255, 127,   127, 255, 191,   255, 255, 255,
    };
    // clang-format on

    CHECK_EQ_INT(mat_type(m), CV_8UC3);
    CHECK_EQ_BYTES(m->data, expected, sizeof(expected));

    free_mat(&m);
}

static void test_synthetic_single_pixel(void) {
    struct Mat *m = alloc_mat_synthetic(1, 1);

    static const uchar expected[3] = {0, 0, 0};
    CHECK_EQ_BYTES(m->data, expected, sizeof(expected));

    free_mat(&m);
}

static void test_fast_malloc_overflow(void) {
    CHECK(fast_malloc(SIZE_MAX) == NULL);
    CHECK(fast_malloc(SIZE_MAX - 8) == NULL);
}

static void test_fast_malloc_alignment(void) {
    static const size_t sizes[] = {0, 1, 15, 16, 17, 1000};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        void *p = fast_malloc(sizes[i]);
        CHECK(p != NULL);
        CHECK_EQ_INT((uintptr_t)p % 16, 0);
        fast_free(p);
    }
}

static void test_fast_free_null(void) {
    fast_free(NULL);
    CHECK(1);
}

static void test_alloc_zero_sized(void) {
    static const int dims[][2] = {{0, 0}, {0, 5}, {3, 0}};
    for (int i = 0; i < 3; i++) {
        struct Mat *m = alloc_mat(dims[i][0], dims[i][1], CV_8UC3);
        CHECK(m != NULL);
        if (m == NULL) {
            continue;
        }
        CHECK_EQ_INT(m->rows, dims[i][0]);
        CHECK_EQ_INT(m->cols, dims[i][1]);
        CHECK_EQ_INT(m->size[0], dims[i][0]);
        CHECK_EQ_INT(m->size[1], dims[i][1]);
        CHECK_EQ_INT(m->step[0], 3 * dims[i][1]);
        CHECK_EQ_INT(m->step[1], 3);
        free_mat(&m);
        CHECK(m == NULL);
    }
}

static void test_mat_row(void) {
    struct Mat *m = alloc_mat(3, 4, CV_8UC3);
    CHECK(mat_row(m, 1) == m->data + m->step[0]);
    CHECK(mat_row(m, 2) == m->data + 2 * m->step[0]);
    free_mat(&m);
}

static void test_cv_16s_types(void) {
    CHECK_EQ_INT(CV_16SC1, 3);
    CHECK_EQ_INT(CV_PIXEL_SIZE(CV_16SC1), 2);
    CHECK_EQ_INT(CV_PIXEL_SIZE(CV_16SC4), 8);
}

static jmp_buf assert_jump;
static int     assert_caught;

static void jump_handler(const char *expr, const char *file, int line) {
    (void)expr;
    (void)file;
    (void)line;
    assert_caught++;
    longjmp(assert_jump, 1);
}

static void test_mat_init_layout(void) {
    uchar      buf[24];
    struct Mat m;
    CHECK_EQ_INT(mat_init(&m, 2, 4, CV_8UC3, buf, sizeof(buf)), 0);
    CHECK_EQ_INT(m.rows, 2);
    CHECK_EQ_INT(m.cols, 4);
    CHECK_EQ_INT(m.dims, 2);
    CHECK_EQ_INT(m.size[0], 2);
    CHECK_EQ_INT(m.size[1], 4);
    CHECK_EQ_INT(m.step[0], 12);
    CHECK_EQ_INT(m.step[1], 3);
    CHECK_EQ_INT(mat_type(&m), CV_8UC3);
    CHECK(m.data == buf);
    CHECK(mat_row(&m, 1) == buf + 12);
    CHECK((m.flags & MAT_FLAG_HEAP) == 0);
    CHECK_EQ_INT(m.flags & 0xFFFF0000UL, MAGIC_VAL);
}

static void test_mat_init_rejects_bad_input(void) {
    uchar      buf[24];
    struct Mat m;
    CHECK_EQ_INT(mat_init(&m, 2, 4, CV_8UC3, buf, 23), -1);
    CHECK_EQ_INT(mat_init(&m, -1, 4, CV_8UC3, buf, sizeof(buf)), -1);
    CHECK_EQ_INT(mat_init(&m, 2, -4, CV_8UC3, buf, sizeof(buf)), -1);
    // cols * channels above INT_MAX, even though data_size claims enough.
    CHECK_EQ_INT(mat_init(&m, 1, INT_MAX / 3 + 1, CV_8UC3, buf, SIZE_MAX), -1);
    CHECK_EQ_INT(mat_init(&m, 1, INT_MAX, CV_8UC4, buf, SIZE_MAX), -1);
    CHECK(mat_create(1, INT_MAX / 3 + 1, CV_8UC3) == NULL);
}

static void test_mat_init_zero_sized(void) {
    struct Mat m;
    CHECK_EQ_INT(mat_init(&m, 0, 5, CV_8UC1, NULL, 0), 0);
    CHECK_EQ_INT(m.rows, 0);
    CHECK_EQ_INT(m.step[0], 5);
}

static void test_mat_init_dst_matches_alloc(void) {
    struct Mat *src = alloc_mat(3, 5, CV_8UC3);
    for (size_t i = 0; i < (size_t)src->rows * src->step[0]; i++) {
        src->data[i] = (uchar)(i * 29u + 3u);
    }
    struct Mat *heap_dst = alloc_mat(3, 5, CV_8UC1);
    uchar       buf[15];
    struct Mat  stack_dst;
    CHECK_EQ_INT(mat_init(&stack_dst, 3, 5, CV_8UC1, buf, sizeof(buf)), 0);

    cvt_color(heap_dst, src, COLOR_RGB2GRAY);
    cvt_color(&stack_dst, src, COLOR_RGB2GRAY);
    CHECK_EQ_BYTES(buf, heap_dst->data, sizeof(buf));

    free_mat(&heap_dst);
    free_mat(&src);
}

static void test_heap_flag(void) {
    struct Mat *m = alloc_mat(2, 2, CV_8UC3);
    CHECK((m->flags & MAT_FLAG_HEAP) != 0);
    CHECK_EQ_INT(mat_type(m), CV_8UC3);
    CHECK_EQ_INT(mat_channel_num(m), 3);
    CHECK_EQ_INT(mat_depth(m), CV_8U);
    free_mat(&m);
}

static void test_alloc_mat_two_allocations(void) {
    alloc_counter_start();
    struct Mat *m = alloc_mat(4, 4, CV_8UC3);
    CHECK_EQ_INT(alloc_stats.allocs, 2);
    free_mat(&m);
    alloc_counter_stop();
    CHECK_EQ_INT(alloc_stats.frees, 2);
    CHECK_EQ_INT(alloc_stats.current, 0);
}

static void test_mat_init_misaligned_asserts(void) {
    uchar      buf[32];
    struct Mat m;
    assert_caught = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        mat_init(&m, 1, 2, CV_32FC1, buf + 1, 8);
    }
    improc_set_assert_handler(NULL);
    CHECK_EQ_INT(assert_caught, 1);
}

static void test_free_mat_rejects_non_heap(void) {
    uchar       buf[4];
    struct Mat  m;
    struct Mat *p = &m;
    CHECK_EQ_INT(mat_init(&m, 1, 4, CV_8UC1, buf, sizeof(buf)), 0);
    assert_caught = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        free_mat(&p);
    }
    improc_set_assert_handler(NULL);
    CHECK_EQ_INT(assert_caught, 1);
}

int main(void) {
    RUN_TEST(test_type_macros);
    RUN_TEST(test_alloc_layout);
    RUN_TEST(test_mat_at_is_interleaved);
    RUN_TEST(test_synthetic_pattern);
    RUN_TEST(test_synthetic_single_pixel);
    RUN_TEST(test_fast_malloc_overflow);
    RUN_TEST(test_fast_malloc_alignment);
    RUN_TEST(test_fast_free_null);
    RUN_TEST(test_alloc_zero_sized);
    RUN_TEST(test_mat_row);
    RUN_TEST(test_cv_16s_types);
    RUN_TEST(test_mat_init_layout);
    RUN_TEST(test_mat_init_rejects_bad_input);
    RUN_TEST(test_mat_init_zero_sized);
    RUN_TEST(test_mat_init_dst_matches_alloc);
    RUN_TEST(test_heap_flag);
    RUN_TEST(test_alloc_mat_two_allocations);
    RUN_TEST(test_mat_init_misaligned_asserts);
    RUN_TEST(test_free_mat_rejects_non_heap);
    return test_finish();
}
