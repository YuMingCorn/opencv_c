#ifndef MORPH_H_
#define MORPH_H_

#include "mat.h"

// Rectangular ksize x ksize structuring element, ksize odd in 1..255.
// CV_8UC1/3/4, per channel. dst has the same type and size as src and does not
// share memory with it. Pixels outside the image are ignored: the window for
// (y, x) is clipped to the image, so the border adds no value of its own.
// Scratch memory is one row of cols * channels bytes from fast_malloc.
void erode(struct Mat *dst, const struct Mat *src, int ksize);  // window min
void dilate(struct Mat *dst, const struct Mat *src, int ksize); // window max

#endif // MORPH_H_
