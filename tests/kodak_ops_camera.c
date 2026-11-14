#include "improc.h"
#include "kodak.h"

// Camera-format inputs are packed from the Kodak RGB image here, and
// tools/opencv_ref/camera.py packs them the same way:
//   565: t = (R >> 3) << 11 | (G >> 2) << 5 | B >> 3 (cv2 COLOR_RGB2BGR565),
//        stored little-endian, or big-endian for the BE op.
//   YUYV: for each pixel pair p0, p1: Y0 = p0.R, U = p0.G, Y1 = p1.R,
//        V = p1.B. This is not a colour conversion; it only gives the decoder
//        varied, full-range bytes.

static struct Mat *pack_565(const struct Mat *rgb, int big_endian) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC2);
    for (int y = 0; y < rgb->rows; y++) {
        const uchar *s = mat_row(rgb, y);
        uchar       *d = mat_row(out, y);
        for (int x = 0; x < rgb->cols; x++, s += 3, d += 2) {
            unsigned t  = ((unsigned)(s[0] >> 3) << 11) |
                          ((unsigned)(s[1] >> 2) << 5) | (unsigned)(s[2] >> 3);
            uchar    lo = (uchar)(t & 0xFFu);
            uchar    hi = (uchar)(t >> 8);
            d[0]        = big_endian ? hi : lo;
            d[1]        = big_endian ? lo : hi;
        }
    }
    return out;
}

static struct Mat *pack_yuyv(const struct Mat *rgb) {
    struct Mat *out = alloc_mat(rgb->rows, rgb->cols, CV_8UC2);
    for (int y = 0; y < rgb->rows; y++) {
        const uchar *s = mat_row(rgb, y);
        uchar       *d = mat_row(out, y);
        for (int x = 0; x + 1 < rgb->cols; x += 2, s += 6, d += 4) {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[3];
            d[3] = s[5];
        }
    }
    return out;
}

static struct Mat *decode(struct Mat *packed, int code, int dtype) {
    struct Mat *out = alloc_mat(packed->rows, packed->cols, dtype);
    cvt_color(out, packed, code);
    free_mat(&packed);
    return out;
}

static struct Mat *op_bgr565_rgb(const struct Mat *rgb) {
    return decode(pack_565(rgb, 0), COLOR_BGR5652RGB, CV_8UC3);
}

static struct Mat *op_bgr565be_gray(const struct Mat *rgb) {
    return decode(pack_565(rgb, 1), COLOR_BGR565BE2GRAY, CV_8UC1);
}

static struct Mat *op_yuyv_rgb(const struct Mat *rgb) {
    return decode(pack_yuyv(rgb), COLOR_YUV2RGB_YUYV, CV_8UC3);
}

static struct Mat *op_yuyv_gray(const struct Mat *rgb) {
    return decode(pack_yuyv(rgb), COLOR_YUV2GRAY_YUYV, CV_8UC1);
}

const struct KodakOp kodak_ops[] = {
    {"bgr565_rgb", op_bgr565_rgb},
    {"bgr565be_gray", op_bgr565be_gray},
    {"yuyv_rgb", op_yuyv_rgb},
    {"yuyv_gray", op_yuyv_gray},
};

const int kodak_op_count = (int)(sizeof(kodak_ops) / sizeof(kodak_ops[0]));
