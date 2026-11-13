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
    COLOR_BGRA2RGB = COLOR_RGBA2BGR,
    // CV_8UC3 to CV_8UC3. HSV uses H 0..179, S and V 0..255.
    COLOR_RGB2HSV,
    COLOR_BGR2HSV,
    COLOR_HSV2RGB,
    COLOR_HSV2BGR,
    // CV_8UC3 to CV_8UC3. JFIF full-range BT.601: Y, U (Cb), V (Cr).
    COLOR_RGB2YUV,
    COLOR_BGR2YUV,
    COLOR_YUV2RGB,
    COLOR_YUV2BGR
};

// 8-bit color conversion. The caller allocates dst with the matching type and
// the same rows and cols as src. Gray uses Y = (77R + 150G + 29B + 128) >> 8.
// RGB2HSV matches OpenCV exactly; HSV2RGB and YUV are within 1 level of
// OpenCV (YUV is YCrCb with the two chroma channels swapped).
void cvt_color(struct Mat *dst, const struct Mat *src, int code);

#endif // COLOR_H_
