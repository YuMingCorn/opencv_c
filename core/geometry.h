#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "mat.h"

enum FlipCodes { FLIP_VERTICAL = 0, FLIP_HORIZONTAL = 1, FLIP_BOTH = -1 };

// Flips src into dst (same type and size, CV_8UC1/3/4). FLIP_VERTICAL reverses
// row order, FLIP_HORIZONTAL reverses pixel order within each row.
void flip(struct Mat *dst, const struct Mat *src, int flip_code);

enum InterpolationFlags { INTER_NEAREST, INTER_LINEAR };

// Resizes src into dst (same type, CV_8UC1/3/4). The target size is the size
// of dst. Uses half-pixel centres and integer arithmetic only.
void resize(struct Mat *dst, const struct Mat *src, int interpolation);

#endif // GEOMETRY_H_
