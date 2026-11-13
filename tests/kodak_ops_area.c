#include "improc.h"
#include "kodak.h"

static struct Mat *area(const struct Mat *rgb, int rows, int cols) {
    struct Mat *out = alloc_mat(rows, cols, CV_8UC3);
    resize(out, rgb, INTER_AREA);
    return out;
}

static struct Mat *op_area_half(const struct Mat *rgb) {
    return area(rgb, rgb->rows / 2, rgb->cols / 2);
}

static struct Mat *op_area_third(const struct Mat *rgb) {
    return area(rgb, rgb->rows / 3, rgb->cols / 3);
}

static struct Mat *op_area_quarter(const struct Mat *rgb) {
    return area(rgb, rgb->rows / 4, rgb->cols / 4);
}

static struct Mat *op_area_nonint(const struct Mat *rgb) {
    return area(rgb, rgb->rows * 5 / 12 + 1, rgb->cols * 3 / 7);
}

static struct Mat *op_area_up_1_5(const struct Mat *rgb) {
    return area(rgb, rgb->rows * 3 / 2, rgb->cols * 3 / 2);
}

const struct KodakOp kodak_ops[] = {
    {"area_half", op_area_half},
    {"area_third", op_area_third},
    {"area_quarter", op_area_quarter},
    {"area_nonint", op_area_nonint},
    {"area_up_1_5", op_area_up_1_5},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
