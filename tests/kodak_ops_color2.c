#include "improc.h"
#include "kodak.h"

static struct Mat *convert3(const struct Mat *in, int code) {
    struct Mat *out = alloc_mat(in->rows, in->cols, CV_8UC3);
    cvt_color(out, in, code);
    return out;
}

static struct Mat *op_rgb2hsv(const struct Mat *rgb) {
    return convert3(rgb, COLOR_RGB2HSV);
}

// HSV2RGB applied to our own RGB2HSV output.
static struct Mat *op_hsv2rgb_roundtrip(const struct Mat *rgb) {
    struct Mat *hsv  = convert3(rgb, COLOR_RGB2HSV);
    struct Mat *back = convert3(hsv, COLOR_HSV2RGB);
    free_mat(&hsv);
    return back;
}

static struct Mat *op_rgb2yuv(const struct Mat *rgb) {
    return convert3(rgb, COLOR_RGB2YUV);
}

// YUV2RGB applied to our own RGB2YUV output.
static struct Mat *op_yuv2rgb_roundtrip(const struct Mat *rgb) {
    struct Mat *yuv  = convert3(rgb, COLOR_RGB2YUV);
    struct Mat *back = convert3(yuv, COLOR_YUV2RGB);
    free_mat(&yuv);
    return back;
}

const struct KodakOp kodak_ops[] = {
    {"rgb2hsv", op_rgb2hsv},
    {"hsv2rgb_roundtrip", op_hsv2rgb_roundtrip},
    {"rgb2yuv", op_rgb2yuv},
    {"yuv2rgb_roundtrip", op_yuv2rgb_roundtrip},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
