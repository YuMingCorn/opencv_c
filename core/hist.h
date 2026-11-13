#ifndef HIST_H_
#define HIST_H_

#include <stdint.h>

#include "mat.h"
#include "utils.h"

// Largest pixel count the histogram functions accept. Keeps every running sum
// below 2^24, so products with 255 fit uint32_t.
#define HIST_MAX_PIXELS 16777216UL

// Scratch bytes that equalize_hist and otsu_threshold take from fast_malloc
// (one block, released before return). A NULL block fails ASSERT. Without an
// allocator (IMPROC_NO_MALLOC), call calc_hist and otsu_threshold_hist with a
// caller-owned histogram instead of otsu_threshold.
#define HIST_EQUALIZE_SCRATCH (256 * 4 + 256)
#define HIST_OTSU_SCRATCH     (256 * 4)

// hist[v] = number of pixels of src equal to v. src: CV_8UC1, at most 2^24
// pixels. An empty src gives an all-zero histogram.
void calc_hist(uint32_t hist[256], const struct Mat *src);

// Histogram equalization. src, dst: CV_8UC1, same size, distinct buffers.
// m is the lowest non-empty level and den = total - hist[m]. When den == 0 the
// image is copied. Otherwise lut[v] = 0 for v <= m and
// lut[v] = (sum_v * 255 + den / 2) / den for v > m, where sum_v counts the
// pixels in levels m + 1 .. v. Rounds half up.
void equalize_hist(struct Mat *dst, const struct Mat *src);

// Otsu threshold of a histogram with at most 2^24 pixels in total. Returns the
// lowest t that maximizes the between-class variance, with class 0 = levels
// <= t. The criterion s0^2 / w0 + s1^2 / w1 is compared exactly (integer and
// rational part, 128-bit cross products), so there are no float rounding ties.
// Returns 0 when fewer than two levels are populated.
uchar otsu_threshold_hist(const uint32_t hist[256]);

// otsu_threshold_hist of the histogram of src (CV_8UC1). Pair it with
// threshold(dst, src, t, 255, THRESH_BINARY) to binarize.
uchar otsu_threshold(const struct Mat *src);

#endif // HIST_H_
