#ifndef PIXEL_H_
#define PIXEL_H_

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

#endif // PIXEL_H_
