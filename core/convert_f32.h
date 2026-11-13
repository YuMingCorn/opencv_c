#ifndef CONVERT_F32_H_
#define CONVERT_F32_H_

#include <stdint.h>

#include "mat.h"
#include "utils.h"

#if IMPROC_HAS_FLOAT

// dst = src * alpha + beta, computed per element in single precision as
// (float)s * alpha + beta (no fused multiply-add, no libm).
//
// Supported conversions, n in {1, 3, 4}, dst and src the same size and not the
// same buffer:
//   CV_8UCn  -> CV_32FCn
//   CV_32FCn -> CV_8UCn   (round half up, saturate to 0..255; NaN and -inf -> 0)
//   CV_32FCn -> CV_32FCn
// To 8 bits: !(t > 0) -> 0; t >= 255 -> 255; otherwise truncate and add 1 when
// the fraction is >= 0.5. Zero-sized inputs return without touching dst.
void convert_to(struct Mat *dst, const struct Mat *src, float alpha,
                float beta);

#endif // IMPROC_HAS_FLOAT

#endif // CONVERT_F32_H_
