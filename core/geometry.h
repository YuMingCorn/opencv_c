#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "mat.h"

enum FlipCodes { FLIP_VERTICAL = 0, FLIP_HORIZONTAL = 1, FLIP_BOTH = -1 };

// Flips src into dst (same type and size, CV_8UC1/3/4). FLIP_VERTICAL reverses
// row order, FLIP_HORIZONTAL reverses pixel order within each row.
void flip(struct Mat *dst, const struct Mat *src, int flip_code);

enum InterpolationFlags { INTER_NEAREST, INTER_LINEAR, INTER_AREA };

// Resizes src into dst (same type, CV_8UC1/3/4). The target size is the size
// of dst. Uses half-pixel centres and integer arithmetic only.
//
// INTER_AREA averages the exact source area covered by each destination pixel.
// Per axis, destination index d covers [d*s_len, (d+1)*s_len) and source index
// k covers [k*d_len, (k+1)*d_len), both in units of 1/d_len. A weight is the
// integer overlap of the two intervals, so the weights of one destination
// pixel sum to s_len. The result is (sum of weights*pixels + den/2) / den with
// den = src_cols*src_rows (rounded half up), and it also upsamples.
// INTER_AREA requires src rows*cols <= 2^24 so the sums fit in uint32_t, and
// uses 2*dst_cols*channels*4 bytes of scratch from fast_malloc.
void resize(struct Mat *dst, const struct Mat *src, int interpolation);

#endif // GEOMETRY_H_
