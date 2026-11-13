#ifndef FILTER_H_
#define FILTER_H_

#include "mat.h"

// Normalized ksize x ksize mean filter. CV_8UC1/3/4, dst same type and size.
// ksize is odd, 1..255. Border: reflect 101 (gfedcb|abcdefgh|gfedcba).
void box_filter(struct Mat *dst, const struct Mat *src, int ksize);

// Separable Gaussian with fixed integer weights (sum 256 per axis).
// ksize in {1, 3, 5, 7}. Same types and border as box_filter.
void gaussian_blur(struct Mat *dst, const struct Mat *src, int ksize);

// 3x3 Sobel derivative, no scaling. (dx, dy) is (1, 0) or (0, 1).
// src CV_8UC1, dst CV_16SC1 of the same size. Output range is +-1020.
void sobel(struct Mat *dst, const struct Mat *src, int dx, int dy);

#endif // FILTER_H_
