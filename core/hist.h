#ifndef HIST_H_
#define HIST_H_

#include <stdint.h>

#include "mat.h"
#include "utils.h"

// Largest pixel count the histogram functions accept. Keeps every running sum
// below 2^24, so products with 255 fit uint32_t.
#define HIST_MAX_PIXELS 16777216UL

// hist[v] = number of pixels of src equal to v. src: CV_8UC1, at most 2^24
// pixels. An empty src gives an all-zero histogram.
void calc_hist(uint32_t hist[256], const struct Mat *src);

#endif // HIST_H_
