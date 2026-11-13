#include <stdio.h>
#include <stdlib.h>

#include "mat_io.h"
#include "test_mat_util.h"
#include "test_util.h"

static void check_png_round_trip(int rows, int cols, int type,
                                 const char *name) {
    struct Mat *src = alloc_mat(rows, cols, type);
    fill_pattern(src, 5);
    CHECK_EQ_INT(mat_write(name, src), 0);

    struct Mat *back = alloc_mat_from_file(name);
    CHECK(back != NULL);
    if (back != NULL) {
        CHECK_EQ_INT(back->rows, rows);
        CHECK_EQ_INT(back->cols, cols);
        CHECK_EQ_INT(mat_type(back), type);
        if (back->rows == rows && back->cols == cols &&
            mat_type(back) == (size_t)type) {
            CHECK_MAT_BYTES(back, src->data);
        }
    }
    free_mat(&back);
    free_mat(&src);
    remove(name);
}

static void test_png_round_trip_c1(void) {
    check_png_round_trip(3, 2, CV_8UC1, "io_c1.png");
}

static void test_png_round_trip_c2(void) {
    check_png_round_trip(2, 2, CV_8UC2, "io_c2.png");
}

static void test_png_round_trip_c3(void) {
    check_png_round_trip(2, 3, CV_8UC3, "io_c3.png");
}

static void test_png_round_trip_c4(void) {
    check_png_round_trip(2, 2, CV_8UC4, "io_c4.png");
}

static void test_uppercase_extension(void) {
    check_png_round_trip(2, 3, CV_8UC3, "io_upper.PNG");
}

static void test_jpg_write(void) {
    struct Mat *src = alloc_mat_synthetic(8, 8);
    CHECK_EQ_INT(mat_write("io_out.jpg", src), 0);

    struct Mat *back = alloc_mat_from_file("io_out.jpg");
    CHECK(back != NULL);
    if (back != NULL) {
        CHECK_EQ_INT(back->rows, 8);
        CHECK_EQ_INT(back->cols, 8);
        CHECK_EQ_INT(mat_type(back), CV_8UC3);
    }
    free_mat(&back);
    free_mat(&src);
    remove("io_out.jpg");
}

static void test_write_bad_extension(void) {
    struct Mat *src = alloc_mat(2, 2, CV_8UC3);
    fill_pattern(src, 1);
    CHECK_EQ_INT(mat_write("x.xyz", src), -1);
    CHECK_EQ_INT(mat_write("noextension", src), -1);
    free_mat(&src);
}

static void test_write_bad_directory(void) {
    struct Mat *src = alloc_mat(2, 2, CV_8UC3);
    fill_pattern(src, 1);
    CHECK_EQ_INT(mat_write("no_such_dir_xyz/out.png", src), -1);
    free_mat(&src);
}

static void test_load_missing_file(void) {
    CHECK(alloc_mat_from_file("no_such_file_xyz.png") == NULL);
}

static void test_load_from_buffer(void) {
    struct Mat *src = alloc_mat(2, 3, CV_8UC3);
    fill_pattern(src, 9);
    CHECK_EQ_INT(mat_write("io_buf.png", src), 0);

    FILE *f = fopen("io_buf.png", "rb");
    CHECK(f != NULL);
    if (f != NULL) {
        uchar  buf[4096];
        size_t n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        CHECK(n > 0 && n < sizeof(buf));

        struct Mat *back = alloc_mat_from_buffer(buf, n);
        CHECK(back != NULL);
        if (back != NULL) {
            CHECK_EQ_INT(back->rows, 2);
            CHECK_EQ_INT(back->cols, 3);
            CHECK_EQ_INT(mat_type(back), CV_8UC3);
            CHECK_MAT_BYTES(back, src->data);
        }
        free_mat(&back);
    }
    free_mat(&src);
    remove("io_buf.png");
}

static void test_load_garbage_buffer(void) {
    static const uchar garbage[16] = {1, 2,  3,  4,  5,  6,  7,  8,
                                      9, 10, 11, 12, 13, 14, 15, 16};
    CHECK(alloc_mat_from_buffer(garbage, sizeof(garbage)) == NULL);
}

int main(void) {
    RUN_TEST(test_png_round_trip_c1);
    RUN_TEST(test_png_round_trip_c2);
    RUN_TEST(test_png_round_trip_c3);
    RUN_TEST(test_png_round_trip_c4);
    RUN_TEST(test_uppercase_extension);
    RUN_TEST(test_jpg_write);
    RUN_TEST(test_write_bad_extension);
    RUN_TEST(test_write_bad_directory);
    RUN_TEST(test_load_missing_file);
    RUN_TEST(test_load_from_buffer);
    RUN_TEST(test_load_garbage_buffer);
    return test_finish();
}
