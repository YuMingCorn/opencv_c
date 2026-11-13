#include <setjmp.h>
#include <stdint.h>

#include "improc.h"
#include "test_alloc_util.h"
#include "test_mat_util.h"
#include "test_util.h"

// ---- ASSERT capture --------------------------------------------------------

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
#define EXPECT_ASSERT(stmt)                                                    \
    do {                                                                       \
        caught_count = 0;                                                      \
        improc_set_assert_handler(jump_handler);                               \
        if (setjmp(assert_jump) == 0) {                                        \
            stmt;                                                              \
        }                                                                      \
        improc_set_assert_handler(NULL);                                       \
        if (caught_count != 1) {                                               \
            TEST_FAIL_("expected ASSERT from: %s", #stmt);                     \
        }                                                                      \
    } while (0)

// ---- Naive reference -------------------------------------------------------

// Window min/max over the clipped (2r+1)^2 window, one channel at a time.
static void naive_morph(uchar *out, const uchar *in, int rows, int cols, int cn,
                        int ksize, int is_dilate) {
    const int r = ksize / 2;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            for (int c = 0; c < cn; c++) {
                int m = is_dilate ? 0 : 255;
                for (int yy = y - r; yy <= y + r; yy++) {
                    for (int xx = x - r; xx <= x + r; xx++) {
                        if (yy < 0 || yy >= rows || xx < 0 || xx >= cols) {
                            continue;
                        }
                        int v = in[((size_t)yy * cols + xx) * cn + c];
                        if (is_dilate ? v > m : v < m) {
                            m = v;
                        }
                    }
                }
                out[((size_t)y * cols + x) * cn + c] = (uchar)m;
            }
        }
    }
}

static void morph_bytes(int is_dilate, int rows, int cols, const uchar *in,
                        int ksize, const uchar *expected) {
    struct Mat *src = mat_from_bytes(rows, cols, CV_8UC1, in);
    struct Mat *dst = alloc_mat(rows, cols, CV_8UC1);
    if (is_dilate) {
        dilate(dst, src, ksize);
    } else {
        erode(dst, src, ksize);
    }
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&src);
    free_mat(&dst);
}

// ---- Hand-computed cases ---------------------------------------------------

static const uchar grid[] = {5, 9, 1, 7, 3, 8, 2, 6, 4, 10, 11, 15, 13, 12, 14};

static void test_erode3_grid(void) {
    const uchar out[] = {2, 1, 1, 1, 3, 2, 1, 1, 1, 3, 2, 2, 2, 4, 4};
    morph_bytes(0, 3, 5, grid, 3, out);
}

static void test_dilate3_grid(void) {
    const uchar out[] = {9,  9,  9,  10, 10, 15, 15, 15,
                         14, 14, 15, 15, 15, 14, 14};
    morph_bytes(1, 3, 5, grid, 3, out);
}

static void test_erode5_grid(void) {
    uchar out[15];
    memset(out, 1, sizeof out);
    morph_bytes(0, 3, 5, grid, 5, out);
}

static void test_dilate5_grid(void) {
    const uchar out[] = {15, 15, 15, 15, 14, 15, 15, 15,
                         15, 14, 15, 15, 15, 15, 14};
    morph_bytes(1, 3, 5, grid, 5, out);
}

static void test_ksize1_identity(void) {
    morph_bytes(0, 3, 5, grid, 1, grid);
    morph_bytes(1, 3, 5, grid, 1, grid);
}

static void test_kernel_larger_than_image(void) {
    // 2x2 image, 7x7 window: every pixel sees the whole image.
    const uchar in[] = {4, 9, 2, 6};
    const uchar lo[] = {2, 2, 2, 2};
    const uchar hi[] = {9, 9, 9, 9};
    morph_bytes(0, 2, 2, in, 7, lo);
    morph_bytes(1, 2, 2, in, 7, hi);
}

static void test_outside_pixels_ignored(void) {
    // A zero border would turn every edge pixel of erode into 0, and a 255
    // border would turn dilate into 255. Neither may happen.
    uchar in[16];
    memset(in, 100, sizeof in);
    morph_bytes(0, 4, 4, in, 3, in);
    morph_bytes(1, 4, 4, in, 3, in);
}

static void test_rgb_channels_independent(void) {
    // 1x3 RGB, ksize 3: each channel is reduced on its own.
    const uchar in[] = {10, 200, 30, 20, 100, 90, 5, 150, 60};
    const uchar lo[] = {10, 100, 30, 5, 100, 30, 5, 100, 60};
    const uchar hi[] = {20, 200, 90, 20, 200, 90, 20, 150, 90};
    struct Mat *src  = mat_from_bytes(1, 3, CV_8UC3, in);
    struct Mat *dst  = alloc_mat(1, 3, CV_8UC3);
    erode(dst, src, 3);
    CHECK_MAT_BYTES(dst, lo);
    dilate(dst, src, 3);
    CHECK_MAT_BYTES(dst, hi);
    free_mat(&src);
    free_mat(&dst);
}

static void test_non_contiguous_roi(void) {
    // Header over a wider buffer: the step is larger than cols * channels.
    uchar buf[3 * 8];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = (uchar)(i * 53u + 7u);
    }
    struct Mat src;
    CHECK_EQ_INT(mat_init(&src, 3, 4, CV_8UC1, buf, sizeof buf), 0);
    src.step[0] = 8;
    uchar dbuf[3 * 8];
    memset(dbuf, 0xAA, sizeof dbuf);
    struct Mat dst;
    CHECK_EQ_INT(mat_init(&dst, 3, 4, CV_8UC1, dbuf, sizeof dbuf), 0);
    dst.step[0] = 8;

    uchar packed[12];
    for (int y = 0; y < 3; y++) {
        memcpy(packed + y * 4, buf + y * 8, 4);
    }
    uchar expect[12];
    naive_morph(expect, packed, 3, 4, 1, 3, 0);
    erode(&dst, &src, 3);
    for (int y = 0; y < 3; y++) {
        CHECK_EQ_BYTES(dbuf + y * 8, expect + y * 4, 4);
        for (int i = 4; i < 8; i++) {
            CHECK_EQ_INT(dbuf[y * 8 + i], 0xAA); // padding untouched
        }
    }
}

// ---- Sweep against the naive reference -------------------------------------

static void sweep_one(int type, int rows, int cols, int ksize, int is_dilate) {
    const int   cn  = (int)CV_MAT_CN(type);
    struct Mat *src = alloc_mat(rows, cols, type);
    struct Mat *dst = alloc_mat(rows, cols, type);
    fill_pattern(src, (unsigned)(rows * 7 + cols));
    const size_t bytes  = (size_t)rows * (size_t)cols * (size_t)cn;
    uchar       *expect = (uchar *)malloc(bytes + 1);
    naive_morph(expect, src->data, rows, cols, cn, ksize, is_dilate);
    if (is_dilate) {
        dilate(dst, src, ksize);
    } else {
        erode(dst, src, ksize);
    }
    if (bytes > 0 && memcmp(dst->data, expect, bytes) != 0) {
        TEST_FAIL_("mismatch type=%d rows=%d cols=%d ksize=%d dilate=%d", type,
                   rows, cols, ksize, is_dilate);
    }
    free(expect);
    free_mat(&src);
    free_mat(&dst);
}

static void test_width_sweep_0_to_65(void) {
    const int types[]  = {CV_8UC1, CV_8UC3, CV_8UC4};
    const int ksizes[] = {1, 3, 5, 7, 9};
    for (int t = 0; t < 3; t++) {
        for (int cols = 0; cols <= 65; cols++) {
            for (int k = 0; k < 5; k++) {
                for (int d = 0; d < 2; d++) {
                    sweep_one(types[t], 6, cols, ksizes[k], d);
                }
            }
        }
    }
}

static void test_rows_sweep_and_large_ksize(void) {
    const int ksizes[] = {1, 3, 7, 15, 255};
    for (int rows = 0; rows <= 20; rows++) {
        for (int k = 0; k < 5; k++) {
            for (int d = 0; d < 2; d++) {
                sweep_one(CV_8UC1, rows, 11, ksizes[k], d);
                sweep_one(CV_8UC3, rows, 5, ksizes[k], d);
            }
        }
    }
}

// ---- Empty input, misuse, scratch -------------------------------------------

static void test_zero_size_leaves_dst_untouched(void) {
    const int dims[][2] = {{0, 5}, {5, 0}, {0, 0}};
    for (int i = 0; i < 3; i++) {
        struct Mat src, dst;
        uchar      sbuf[4] = {1, 2, 3, 4};
        uchar      dbuf[4] = {9, 9, 9, 9};
        CHECK_EQ_INT(mat_init(&src, dims[i][0], dims[i][1], CV_8UC1, sbuf, 4),
                     0);
        CHECK_EQ_INT(mat_init(&dst, dims[i][0], dims[i][1], CV_8UC1, dbuf, 4),
                     0);
        erode(&dst, &src, 3);
        dilate(&dst, &src, 3);
        const uchar keep[4] = {9, 9, 9, 9};
        CHECK_EQ_BYTES(dbuf, keep, 4);
    }
}

static void test_misuse_asserts(void) {
    struct Mat *src = alloc_mat(4, 4, CV_8UC1);
    struct Mat *dst = alloc_mat(4, 4, CV_8UC1);
    struct Mat *bad = alloc_mat(4, 5, CV_8UC1);
    struct Mat *s16 = alloc_mat(4, 4, CV_16SC1);
    struct Mat *rgb = alloc_mat(4, 4, CV_8UC3);
    EXPECT_ASSERT(erode(dst, src, 2));
    EXPECT_ASSERT(dilate(dst, src, 4));
    EXPECT_ASSERT(erode(dst, src, 0));
    EXPECT_ASSERT(erode(dst, src, -3));
    EXPECT_ASSERT(dilate(dst, src, 257));
    EXPECT_ASSERT(erode(dst, src, 256));
    EXPECT_ASSERT(erode(bad, src, 3));
    EXPECT_ASSERT(erode(rgb, src, 3));
    EXPECT_ASSERT(dilate(s16, s16, 3));
    EXPECT_ASSERT(erode(src, src, 3));
    EXPECT_ASSERT(erode(NULL, src, 3));
    free_mat(&rgb);
    free_mat(&s16);
    free_mat(&bad);
    free_mat(&dst);
    free_mat(&src);
}

static void *failing_alloc(size_t size) {
    (void)size;
    return NULL;
}

static void failing_free(void *ptr) { (void)ptr; }

static void test_allocation_failure_asserts(void) {
    struct Mat *src = alloc_mat(4, 4, CV_8UC1);
    struct Mat *dst = alloc_mat(4, 4, CV_8UC1);
    improc_set_allocator(failing_alloc, failing_free);
    EXPECT_ASSERT(erode(dst, src, 3));
    improc_set_allocator(NULL, NULL);
    free_mat(&dst);
    free_mat(&src);
}

static void test_scratch_is_one_row_and_balanced(void) {
    struct Mat *src = alloc_mat(40, 64, CV_8UC3);
    struct Mat *dst = alloc_mat(40, 64, CV_8UC3);
    fill_pattern(src, 5);

    alloc_counter_start();
    erode(dst, src, 7);
    dilate(dst, src, 255);
    alloc_counter_stop();

    CHECK_EQ_INT(alloc_stats.allocs, 2);
    CHECK_EQ_INT(alloc_stats.allocs, alloc_stats.frees);
    CHECK_EQ_INT(alloc_stats.current, 0);
    CHECK(alloc_stats.peak >= 64 * 3 && alloc_stats.peak <= 64 * 3 + 64); // one row
    free_mat(&dst);
    free_mat(&src);
}

// ---- Opening and closing ---------------------------------------------------

static void open_close_run(int is_close, struct Mat *dst, struct Mat *src,
                           int ksize) {
    struct Mat *tmp = alloc_mat(src->rows, src->cols, (int)mat_type(src));
    if (is_close) {
        morph_close(dst, tmp, src, ksize);
    } else {
        morph_open(dst, tmp, src, ksize);
    }
    free_mat(&tmp);
}

static void test_open3_block_survives_lone_pixel_removed(void) {
    uchar in[5 * 6];
    memset(in, 0, sizeof in);
    for (int y = 1; y <= 3; y++) {
        for (int x = 1; x <= 3; x++) {
            in[y * 6 + x] = 255;
        }
    }
    in[2 * 6 + 5] = 255; // lone pixel
    uchar out[5 * 6];
    memcpy(out, in, sizeof out);
    out[2 * 6 + 5] = 0;
    struct Mat *src = mat_from_bytes(5, 6, CV_8UC1, in);
    struct Mat *dst = alloc_mat(5, 6, CV_8UC1);
    open_close_run(0, dst, src, 3);
    CHECK_MAT_BYTES(dst, out);
    free_mat(&src);
    free_mat(&dst);
}

static void test_close3_fills_holes(void) {
    uchar in[4 * 5];
    memset(in, 255, sizeof in);
    in[1 * 5 + 2] = 0;
    in[3 * 5 + 0] = 0;
    uchar out[4 * 5];
    memset(out, 255, sizeof out);
    struct Mat *src = mat_from_bytes(4, 5, CV_8UC1, in);
    struct Mat *dst = alloc_mat(4, 5, CV_8UC1);
    open_close_run(1, dst, src, 3);
    CHECK_MAT_BYTES(dst, out);
    free_mat(&src);
    free_mat(&dst);
}

static void test_open_close_match_naive_composition(void) {
    const int ksizes[] = {1, 3, 5, 7};
    for (int cols = 0; cols <= 65; cols += 4) {
        for (int k = 0; k < 4; k++) {
            for (int cn = 1; cn <= 4; cn += (cn == 1 ? 2 : 1)) {
                const int   type = CV_MAKETYPE(CV_8U, cn);
                const int   rows = 7;
                struct Mat *src  = alloc_mat(rows, cols, type);
                struct Mat *dst  = alloc_mat(rows, cols, type);
                fill_pattern(src, (unsigned)cols);
                const size_t bytes = (size_t)rows * (size_t)cols * (size_t)cn;
                uchar       *a     = (uchar *)malloc(bytes + 1);
                uchar       *b     = (uchar *)malloc(bytes + 1);
                for (int is_close = 0; is_close < 2; is_close++) {
                    naive_morph(a, src->data, rows, cols, cn, ksizes[k],
                                is_close);
                    naive_morph(b, a, rows, cols, cn, ksizes[k], !is_close);
                    open_close_run(is_close, dst, src, ksizes[k]);
                    if (bytes > 0 && memcmp(dst->data, b, bytes) != 0) {
                        TEST_FAIL_("mismatch cn=%d cols=%d ksize=%d close=%d",
                                   cn, cols, ksizes[k], is_close);
                    }
                }
                free(a);
                free(b);
                free_mat(&src);
                free_mat(&dst);
            }
        }
    }
}

static void test_open_close_properties(void) {
    // Opening is anti-extensive and closing is extensive; both are idempotent.
    struct Mat *src = alloc_mat(13, 17, CV_8UC1);
    struct Mat *o1  = alloc_mat(13, 17, CV_8UC1);
    struct Mat *o2  = alloc_mat(13, 17, CV_8UC1);
    struct Mat *c1  = alloc_mat(13, 17, CV_8UC1);
    struct Mat *c2  = alloc_mat(13, 17, CV_8UC1);
    fill_pattern(src, 11);
    open_close_run(0, o1, src, 3);
    open_close_run(0, o2, o1, 3);
    open_close_run(1, c1, src, 3);
    open_close_run(1, c2, c1, 3);
    CHECK_MAT_BYTES(o2, o1->data);
    CHECK_MAT_BYTES(c2, c1->data);
    for (int i = 0; i < 13 * 17; i++) {
        CHECK(o1->data[i] <= src->data[i]);
        CHECK(c1->data[i] >= src->data[i]);
    }
    free_mat(&c2);
    free_mat(&c1);
    free_mat(&o2);
    free_mat(&o1);
    free_mat(&src);
}

static void test_open_close_zero_size_and_misuse(void) {
    struct Mat e_src, e_dst, e_tmp;
    uchar      sb[4] = {1, 2, 3, 4}, db[4] = {9, 9, 9, 9}, tb[4] = {7, 7, 7, 7};
    CHECK_EQ_INT(mat_init(&e_src, 0, 4, CV_8UC1, sb, 4), 0);
    CHECK_EQ_INT(mat_init(&e_dst, 0, 4, CV_8UC1, db, 4), 0);
    CHECK_EQ_INT(mat_init(&e_tmp, 0, 4, CV_8UC1, tb, 4), 0);
    morph_open(&e_dst, &e_tmp, &e_src, 3);
    morph_close(&e_dst, &e_tmp, &e_src, 3);
    CHECK_EQ_INT(db[0], 9);
    CHECK_EQ_INT(tb[0], 7);

    struct Mat *src = alloc_mat(4, 4, CV_8UC1);
    struct Mat *dst = alloc_mat(4, 4, CV_8UC1);
    struct Mat *tmp = alloc_mat(4, 4, CV_8UC1);
    struct Mat *bad = alloc_mat(4, 5, CV_8UC1);
    struct Mat *rgb = alloc_mat(4, 4, CV_8UC3);
    EXPECT_ASSERT(morph_open(dst, tmp, src, 2));
    EXPECT_ASSERT(morph_close(dst, tmp, src, 0));
    EXPECT_ASSERT(morph_open(dst, src, src, 3));  // tmp == src
    EXPECT_ASSERT(morph_close(dst, dst, src, 3)); // tmp == dst
    EXPECT_ASSERT(morph_open(src, tmp, src, 3));  // dst == src
    EXPECT_ASSERT(morph_open(dst, bad, src, 3));  // tmp size
    EXPECT_ASSERT(morph_open(dst, rgb, src, 3));  // tmp type
    EXPECT_ASSERT(morph_close(bad, tmp, src, 3)); // dst size
    EXPECT_ASSERT(morph_close(dst, NULL, src, 3));
    free_mat(&rgb);
    free_mat(&bad);
    free_mat(&tmp);
    free_mat(&dst);
    free_mat(&src);
}


int main(void) {
    RUN_TEST(test_erode3_grid);
    RUN_TEST(test_dilate3_grid);
    RUN_TEST(test_erode5_grid);
    RUN_TEST(test_dilate5_grid);
    RUN_TEST(test_ksize1_identity);
    RUN_TEST(test_kernel_larger_than_image);
    RUN_TEST(test_outside_pixels_ignored);
    RUN_TEST(test_rgb_channels_independent);
    RUN_TEST(test_non_contiguous_roi);
    RUN_TEST(test_width_sweep_0_to_65);
    RUN_TEST(test_rows_sweep_and_large_ksize);
    RUN_TEST(test_zero_size_leaves_dst_untouched);
    RUN_TEST(test_misuse_asserts);
    RUN_TEST(test_allocation_failure_asserts);
    RUN_TEST(test_scratch_is_one_row_and_balanced);
    RUN_TEST(test_open3_block_survives_lone_pixel_removed);
    RUN_TEST(test_close3_fills_holes);
    RUN_TEST(test_open_close_match_naive_composition);
    RUN_TEST(test_open_close_properties);
    RUN_TEST(test_open_close_zero_size_and_misuse);
    return test_finish();
}
