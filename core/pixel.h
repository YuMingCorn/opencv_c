#ifndef PIXEL_H_
#define PIXEL_H_

#include "mat.h"
#include "utils.h"

enum ThresholdTypes { THRESH_BINARY };

void threshold(struct Mat *dst, struct Mat *src, double thresh, double maxval,
               int type);

#endif // PIXEL_H_
