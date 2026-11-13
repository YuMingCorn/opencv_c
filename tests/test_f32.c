#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "improc.h"
#include "test_mat_util.h"
#include "test_util.h"

#if !IMPROC_HAS_FLOAT

int main(void) {
    printf("Skipped: built with IMPROC_ENABLE_FLOAT=OFF\n");
    return TEST_EXIT_SKIP;
}

#else

static jmp_buf assert_jump;
static int     caught_count;

static void jump_handler(const char *expr, const char *file, int line) {
    (void)expr;
    (void)file;
    (void)line;
    caught_count++;
    longjmp(assert_jump, 1);
}

static float float_at(const struct Mat *m, int y, int i) {
    float v;
    memcpy(&v, mat_row(m, y) + 4 * (size_t)i, sizeof v);
    return v;
}

static int type_of(int depth, int cn) { return CV_MAKETYPE(depth, cn); }

static void test_u8_to_f32_hand(void) {
    const uchar in[4]       = {0, 1, 128, 255};
    const float expected[4] = {-1.5f, -1.0f, 62.5f, 126.0f};
    struct Mat *src         = mat_from_bytes(1, 4, CV_8UC1, in);
    struct Mat *dst         = alloc_mat(1, 4, CV_32FC1);
    convert_to(dst, src, 0.5f, -1.5f);
    for (int i = 0; i < 4; i++) {
        CHECK(float_at(dst, 0, i) == expected[i]);
    }
    free_mat(&dst);
    free_mat(&src);
}

static void test_f32_to_u8_rounding(void) {
    const float nan_v        = 0.0f / 0.0f;
    const float inf_v        = 1.0f / 0.0f;
    const float in[13]       = {-3.0f,  0.4f,   0.5f,       1.5f,   2.5f,
                                254.4f, 254.5f, 255.0f,     300.0f, nan_v,
                                inf_v,  -inf_v, 0.49999997f};
    const uchar expected[13] = {0,   0,   1, 2,   3, 254, 255,
                                255, 255, 0, 255, 0, 0};
    struct Mat *src          = mat_from_bytes(1, 13, CV_32FC1, in);
    struct Mat *dst          = alloc_mat(1, 13, CV_8UC1);
    convert_to(dst, src, 1.0f, 0.0f);
    CHECK_MAT_BYTES(dst, expected);
    free_mat(&dst);
    free_mat(&src);
}

static void test_f32_to_f32(void) {
    const float in[3] = {1.0f, -2.0f, 0.25f};
    struct Mat *src   = mat_from_bytes(1, 3, CV_32FC1, in);
    struct Mat *dst   = alloc_mat(1, 3, CV_32FC1);
    convert_to(dst, src, 2.0f, 0.5f);
    CHECK(float_at(dst, 0, 0) == 2.5f);
    CHECK(float_at(dst, 0, 1) == -3.5f);
    CHECK(float_at(dst, 0, 2) == 1.0f);
    free_mat(&dst);
    free_mat(&src);
}

// u8 -> f32 (1/255) -> u8 (255) returns every one of the 256 values.
static void test_roundtrip_identity_all_levels(void) {
    uchar all[256];
    for (int i = 0; i < 256; i++) {
        all[i] = (uchar)i;
    }
    struct Mat *src  = mat_from_bytes(1, 256, CV_8UC1, all);
    struct Mat *mid  = alloc_mat(1, 256, CV_32FC1);
    struct Mat *back = alloc_mat(1, 256, CV_8UC1);
    convert_to(mid, src, 1.0f / 255.0f, 0.0f);
    convert_to(back, mid, 255.0f, 0.0f);
    CHECK_MAT_BYTES(back, all);
    free_mat(&back);
    free_mat(&mid);
    free_mat(&src);
}

// Reference: float product already formed by the caller, then round half up
// through a double.
static uchar ref_to_u8(float t) {
    if (!(t > 0.0f)) {
        return 0;
    }
    double r = (double)t + 0.5;
    if (r >= 255.0) {
        return 255;
    }
    return (uchar)(int)r;
}

static void sweep_one(int cols, int cn) {
    const int   n    = cols * cn;
    const int   rows = 3;
    struct Mat *u8   = alloc_mat(rows, cols, type_of(CV_8U, cn));
    struct Mat *f    = alloc_mat(rows, cols, type_of(CV_32F, cn));
    struct Mat *g    = alloc_mat(rows, cols, type_of(CV_32F, cn));
    struct Mat *u8b  = alloc_mat(rows, cols, type_of(CV_8U, cn));
    fill_pattern(u8, (unsigned)cols);
    const float alpha = 0.37f, beta = -7.25f;

    convert_to(f, u8, alpha, beta);
    convert_to(u8b, f, 3.1f, 20.0f);
    convert_to(g, f, 3.1f, 20.0f);
    for (int y = 0; y < rows; y++) {
        for (int i = 0; i < n; i++) {
            // Store each product in a volatile so the compiler cannot fuse
            // it with the add (Apple clang contracts to FMA by default);
            // the library is built with -ffp-contract=off.
            volatile float p0   = (float)mat_row(u8, y)[i] * alpha;
            float          want = p0 + beta;
            CHECK(float_at(f, y, i) == want);
            volatile float p1 = float_at(f, y, i) * 3.1f;
            float          t  = p1 + 20.0f;
            CHECK(float_at(g, y, i) == t);
            CHECK_EQ_INT(mat_row(u8b, y)[i], ref_to_u8(t));
        }
    }
    free_mat(&u8b);
    free_mat(&g);
    free_mat(&f);
    free_mat(&u8);
}

static void test_width_sweep(void) {
    const int cns[3] = {1, 3, 4};
    for (int cols = 1; cols <= 65; cols++) {
        for (int k = 0; k < 3; k++) {
            sweep_one(cols, cns[k]);
        }
    }
}

static void test_zero_sized_untouched(void) {
    float      src_buf[4] = {0};
    float      dst_buf[4] = {9.0f, 9.0f, 9.0f, 9.0f};
    struct Mat s, d;
    CHECK_EQ_INT(mat_init(&s, 0, 4, CV_32FC1, src_buf, sizeof src_buf), 0);
    CHECK_EQ_INT(mat_init(&d, 0, 4, CV_32FC1, dst_buf, sizeof dst_buf), 0);
    convert_to(&d, &s, 1.0f, 1.0f);
    CHECK(dst_buf[0] == 9.0f && dst_buf[3] == 9.0f);
    CHECK_EQ_INT(mat_init(&s, 4, 0, CV_32FC1, src_buf, sizeof src_buf), 0);
    CHECK_EQ_INT(mat_init(&d, 4, 0, CV_32FC1, dst_buf, sizeof dst_buf), 0);
    convert_to(&d, &s, 1.0f, 1.0f);
    CHECK(dst_buf[0] == 9.0f && dst_buf[3] == 9.0f);
}

static int misuse_asserts(struct Mat *dst, struct Mat *src) {
    caught_count = 0;
    improc_set_assert_handler(jump_handler);
    if (setjmp(assert_jump) == 0) {
        convert_to(dst, src, 1.0f, 0.0f);
    }
    improc_set_assert_handler(NULL);
    return caught_count == 1;
}

static void test_misuse_asserts(void) {
    struct Mat *u8  = alloc_mat(2, 3, CV_8UC1);
    struct Mat *u8b = alloc_mat(2, 3, CV_8UC1);
    struct Mat *f   = alloc_mat(2, 3, CV_32FC1);
    struct Mat *f3  = alloc_mat(2, 3, CV_32FC3);
    struct Mat *fs  = alloc_mat(3, 3, CV_32FC1);
    struct Mat *c2  = alloc_mat(2, 3, CV_8UC2);
    struct Mat *f2  = alloc_mat(2, 3, CV_32FC2);
    struct Mat *s16 = alloc_mat(2, 3, CV_16SC1);

    CHECK(misuse_asserts(u8b, u8)); // u8 -> u8 unsupported
    CHECK(misuse_asserts(f3, u8));  // channel mismatch
    CHECK(misuse_asserts(fs, u8));  // size mismatch
    CHECK(misuse_asserts(f2, c2));  // 2 channels unsupported
    CHECK(misuse_asserts(f, s16));  // 16S source unsupported
    CHECK(misuse_asserts(f, f));    // in place
    CHECK(misuse_asserts(u8, s16)); // 16S source unsupported
    free_mat(&s16);
    free_mat(&f2);
    free_mat(&c2);
    free_mat(&fs);
    free_mat(&f3);
    free_mat(&f);
    free_mat(&u8b);
    free_mat(&u8);
}

int main(void) {
    RUN_TEST(test_u8_to_f32_hand);
    RUN_TEST(test_f32_to_u8_rounding);
    RUN_TEST(test_f32_to_f32);
    RUN_TEST(test_roundtrip_identity_all_levels);
    RUN_TEST(test_width_sweep);
    RUN_TEST(test_zero_sized_untouched);
    RUN_TEST(test_misuse_asserts);
    return test_finish();
}

#endif // IMPROC_HAS_FLOAT
