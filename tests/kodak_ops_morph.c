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

// lane-b-open-close-ops

const struct KodakOp kodak_ops[] = {
    {"erode3", op_erode3},
    {"dilate3", op_dilate3},
    // lane-b-open-close-table
    {"gray_src", op_gray_src},
    {"gray_erode7", op_gray_erode7},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
