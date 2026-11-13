#include "improc.h"
#include "kodak.h"

// Project gray image; the input of every other op in this module.
static struct Mat *op_gray(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC1);
    cvt_color(out, rgb, COLOR_RGB2GRAY);
    return out;
}

// 1 x 256 CV_32S histogram of the gray image.
static struct Mat *op_hist(const struct Mat *rgb) {
    struct Mat *gray = op_gray(rgb);
    struct Mat *out  = alloc_mat(1, 256, CV_MAKETYPE(CV_32S, 1));
    uint32_t    hist[256];
    calc_hist(hist, gray);
    for (int i = 0; i < 256; i++) {
        ((int32_t *)out->data)[i] = (int32_t)hist[i];
    }
    free_mat(&gray);
    return out;
}

const struct KodakOp kodak_ops[] = {
    {"gray", op_gray},
    {"hist", op_hist},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
