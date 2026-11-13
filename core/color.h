#ifndef COLOR_H_
#define COLOR_H_

#include "mat.h"
#include "utils.h"

enum ColorConversionCodes { COLOR_RGB2GRAY, COLOR_RGB2BGR };

void cvt_color(struct Mat *dst, const struct Mat *src, int code);

void rgb_to_bgr(struct Mat *dst, const struct Mat *src);
void rgb_to_gray(struct Mat *dst, const struct Mat *src);

#endif // COLOR_H_
