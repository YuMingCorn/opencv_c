#ifndef COLOR_H_
#define COLOR_H_

#include "mat.h"
#include "utils.h"

enum ColorConversionCodes {
    COLOR_RGB2GRAY,
    COLOR_BGR2GRAY,
    COLOR_RGBA2GRAY,
    COLOR_BGRA2GRAY,
    COLOR_GRAY2RGB,
    COLOR_GRAY2BGR = COLOR_GRAY2RGB,
    COLOR_GRAY2RGBA,
    COLOR_GRAY2BGRA = COLOR_GRAY2RGBA,
    COLOR_RGB2BGR,
    COLOR_BGR2RGB = COLOR_RGB2BGR,
    COLOR_RGBA2BGRA,
    COLOR_BGRA2RGBA = COLOR_RGBA2BGRA,
    COLOR_RGB2RGBA,
    COLOR_BGR2BGRA = COLOR_RGB2RGBA,
    COLOR_RGBA2RGB,
    COLOR_BGRA2BGR = COLOR_RGBA2RGB,
    COLOR_RGB2BGRA,
    COLOR_BGR2RGBA = COLOR_RGB2BGRA,
    COLOR_RGBA2BGR,
    COLOR_BGRA2RGB = COLOR_RGBA2BGR
};

// 8-bit color conversion. The caller allocates dst with the matching type and
// the same rows and cols as src. Gray uses Y = (77R + 150G + 29B + 128) >> 8.
void cvt_color(struct Mat *dst, const struct Mat *src, int code);

#endif // COLOR_H_
