#include "improc.h"
#include "kodak.h"

static struct Mat *op_gray(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC1);
    cvt_color(out, rgb, COLOR_RGB2GRAY);
    return out;
}

static struct Mat *op_bgr(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    cvt_color(out, rgb, COLOR_RGB2BGR);
    return out;
}

static struct Mat *op_rgba(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC4);
    cvt_color(out, rgb, COLOR_RGB2RGBA);
    return out;
}

static struct Mat *op_thresh128(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    threshold(out, rgb, 128, 255, THRESH_BINARY);
    return out;
}

static struct Mat *op_invert(const struct Mat *rgb) {
    uchar lut[256];
    for (int i = 0; i < 256; i++) {
        lut[i] = (uchar)(255 - i);
    }
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    mat_lut(out, rgb, lut);
    return out;
}

static struct Mat *op_bright_384_10(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    convert_scale(out, rgb, 384, 10);
    return out;
}

static struct Mat *op_box5(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    box_filter(out, rgb, 5);
    return out;
}

static struct Mat *gauss(const struct Mat *rgb, int ksize) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    gaussian_blur(out, rgb, ksize);
    return out;
}

static struct Mat *op_gauss3(const struct Mat *rgb) { return gauss(rgb, 3); }
static struct Mat *op_gauss5(const struct Mat *rgb) { return gauss(rgb, 5); }
static struct Mat *op_gauss7(const struct Mat *rgb) { return gauss(rgb, 7); }

// Sobel on the project gray image, then |x| scaled by 1 into 8 bits.
static struct Mat *sobel_abs(const struct Mat *rgb, int dx, int dy) {
    struct Mat *gray = op_gray(rgb);
    struct Mat *grad = alloc_mat(rgb->rows, rgb->cols, CV_16SC1);
    struct Mat *out  = alloc_mat(rgb->rows, rgb->cols, CV_8UC1);
    sobel(grad, gray, dx, dy);
    convert_scale_abs(out, grad, 256, 0);
    free_mat(&grad);
    free_mat(&gray);
    return out;
}

static struct Mat *op_sobel_x_abs(const struct Mat *rgb) {
    return sobel_abs(rgb, 1, 0);
}

static struct Mat *op_sobel_y_abs(const struct Mat *rgb) {
    return sobel_abs(rgb, 0, 1);
}

static struct Mat *flipped(const struct Mat *rgb, int code) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC3);
    flip(out, rgb, code);
    return out;
}

static struct Mat *op_flip_h(const struct Mat *rgb) {
    return flipped(rgb, FLIP_HORIZONTAL);
}

static struct Mat *op_flip_v(const struct Mat *rgb) {
    return flipped(rgb, FLIP_VERTICAL);
}

static struct Mat *op_flip_both(const struct Mat *rgb) {
    return flipped(rgb, FLIP_BOTH);
}

static struct Mat *resized(const struct Mat *rgb, int rows, int cols,
                           int interpolation) {
    struct Mat *out = alloc_mat(rows, cols, CV_8UC3);
    resize(out, rgb, interpolation);
    return out;
}

static struct Mat *op_half_linear(const struct Mat *rgb) {
    return resized(rgb, rgb->rows / 2, rgb->cols / 2, INTER_LINEAR);
}

static struct Mat *op_nonint_linear(const struct Mat *rgb) {
    return resized(rgb, rgb->rows * 5 / 12 + 1, rgb->cols * 3 / 7,
                   INTER_LINEAR);
}

static struct Mat *op_double_nearest(const struct Mat *rgb) {
    return resized(rgb, rgb->rows * 2, rgb->cols * 2, INTER_NEAREST);
}

static struct Mat *op_third_nearest(const struct Mat *rgb) {
    return resized(rgb, rgb->rows / 3, rgb->cols / 3, INTER_NEAREST);
}

const struct KodakOp kodak_ops[] = {
    {"gray", op_gray},
    {"bgr", op_bgr},
    {"rgba", op_rgba},
    {"thresh128", op_thresh128},
    {"invert", op_invert},
    {"bright_384_10", op_bright_384_10},
    {"box5", op_box5},
    {"gauss3", op_gauss3},
    {"gauss5", op_gauss5},
    {"gauss7", op_gauss7},
    {"sobel_x_abs", op_sobel_x_abs},
    {"sobel_y_abs", op_sobel_y_abs},
    {"flip_h", op_flip_h},
    {"flip_v", op_flip_v},
    {"flip_both", op_flip_both},
    {"half_linear", op_half_linear},
    {"nonint_linear", op_nonint_linear},
    {"double_nearest", op_double_nearest},
    {"third_nearest", op_third_nearest},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
