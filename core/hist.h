#ifndef HIST_H_
#define HIST_H_

#include <stdint.h>

#include "mat.h"
#include "utils.h"

// Largest pixel count the histogram functions accept. Keeps every running sum
// below 2^24, so products with 255 fit uint32_t.
#define HIST_MAX_PIXELS 16777216UL

// Scratch bytes that equalize_hist takes from fast_malloc (one block, released
// before return). A NULL block fails ASSERT.
#define HIST_EQUALIZE_SCRATCH (256 * 4 + 256)

// hist[v] = number of pixels of src equal to v. src: CV_8UC1, at most 2^24
// pixels. An empty src gives an all-zero histogram.
void calc_hist(uint32_t hist[256], const struct Mat *src);

// Histogram equalization. src, dst: CV_8UC1, same size, distinct buffers.
// m is the lowest non-empty level and den = total - hist[m]. When den == 0 the
// image is copied. Otherwise lut[v] = 0 for v <= m and
// lut[v] = (sum_v * 255 + den / 2) / den for v > m, where sum_v counts the
// pixels in levels m + 1 .. v. Rounds half up.
void equalize_hist(struct Mat *dst, const struct Mat *src);

#endif // HIST_H_
