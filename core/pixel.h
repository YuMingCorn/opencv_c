#ifndef PIXEL_H_
#define PIXEL_H_

#include <stdint.h>

#include "mat.h"
#include "utils.h"

enum ThresholdTypes {
    THRESH_BINARY,
    THRESH_BINARY_INV,
    THRESH_TRUNC,
    THRESH_TOZERO,
    THRESH_TOZERO_INV
};

// src and dst: CV_8UC1/3/4, same type and size. Every channel is thresholded.
void threshold(struct Mat *dst, const struct Mat *src, uchar thresh,
               uchar maxval, int type);

// dst[i] = lut[src[i]] for every channel. Same type/size rules as threshold.
void mat_lut(struct Mat *dst, const struct Mat *src, const uchar lut[256]);

// dst = saturate_u8(round(src * alpha_q8 / 256 + beta)), CV_8UCn -> CV_8UCn.
// alpha_q8 in [-32768, 32767], beta in [-65536, 65535]. Intermediates are
// int32_t: 255 * 32767 + 65535 * 256 + 128 < 2^25.
void convert_scale(struct Mat *dst, const struct Mat *src, int32_t alpha_q8,
                   int32_t beta);

// dst = saturate_u8(round(|src * alpha_q8 / 256 + beta|)), CV_16SCn -> CV_8UCn.
// Same ranges. 32768 * 32768 + 65536 * 256 < 2^31, so int32_t does not
// overflow.
void convert_scale_abs(struct Mat *dst, const struct Mat *src, int32_t alpha_q8,
                       int32_t beta);

#endif // PIXEL_H_
