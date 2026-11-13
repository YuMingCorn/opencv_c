#include "improc.h"
#include "kodak.h"

static struct Mat *morph_rgb(const struct Mat *rgb, int ksize, int is_dilate) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    if (is_dilate) {
        dilate(out, rgb, ksize);
    } else {
        erode(out, rgb, ksize);
    }
    return out;
}

static struct Mat *op_erode3(const struct Mat *rgb) {
    return morph_rgb(rgb, 3, 0);
}

static struct Mat *op_dilate3(const struct Mat *rgb) {
    return morph_rgb(rgb, 3, 1);
}

static struct Mat *op_gray_erode7(const struct Mat *rgb) {
    struct Mat *gray = alloc_mat(rgb->rows, rgb->cols, CV_8UC1);
    struct Mat *out  = alloc_mat(rgb->rows, rgb->cols, CV_8UC1);
    cvt_color(gray, rgb, COLOR_RGB2GRAY);
    erode(out, gray, 7);
    free_mat(&gray);
    return out;
}

// Project gray image, dumped so OpenCV erodes exactly the same input.
static struct Mat *op_gray_src(const struct Mat *rgb) {
    struct Mat *gray = alloc_mat(rgb->rows, rgb->cols, CV_8UC1);
    cvt_color(gray, rgb, COLOR_RGB2GRAY);
    return gray;
}

static struct Mat *open_close_rgb(const struct Mat *rgb, int ksize, int is_close) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    struct Mat *tmp = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    if (is_close) {
        morph_close(out, tmp, rgb, ksize);
    } else {
        morph_open(out, tmp, rgb, ksize);
    }
    free_mat(&tmp);
    return out;
}

static struct Mat *op_open5(const struct Mat *rgb) {
    return open_close_rgb(rgb, 5, 0);
}

static struct Mat *op_close5(const struct Mat *rgb) {
    return open_close_rgb(rgb, 5, 1);
}


const struct KodakOp kodak_ops[] = {
    {"erode3", op_erode3},
    {"dilate3", op_dilate3},
    {"open5", op_open5},
    {"close5", op_close5},
    {"gray_src", op_gray_src},
    {"gray_erode7", op_gray_erode7},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
