#include "color.h"

void cvt_color(struct Mat *dst, const struct Mat *src, int code) {
    ASSERT(mat_type(src) == CV_8UC3);
    switch (code) {
    case COLOR_RGB2BGR:
        rgb_to_bgr(dst, src);
        break;
    case COLOR_RGB2GRAY:
        rgb_to_gray(dst, src);
        break;
    }
}

void rgb_to_bgr(struct Mat *dst, const struct Mat *src) {
    for (int i = 0; i < src->rows; i++) {
        // RGB(012) -> BGR(210)
        for (int j = 0; j < src->cols; j++) {
            uchar *s = MAT_AT_ELEM_PTR(*src, uchar, i, j);
            uchar *d = MAT_AT_ELEM_PTR(*dst, uchar, i, j);
            *(d + 2) = *(s + 0);
            *(d + 1) = *(s + 1);
            *(d + 0) = *(s + 2);
        }
    }
}

void rgb_to_gray(struct Mat *dst, const struct Mat *src) {
    uchar *s     = (uchar *)src->data;
    uchar *d     = (uchar *)dst->data;
    size_t s_ch  = mat_channel_num(src);
    size_t d_ch  = mat_channel_num(dst);
    int    total = src->rows * src->cols;
    for (int i = 0; i < total; i++) {
        *d = (uchar)(*(s + 0) * 0.299 + *(s + 1) * 0.587 + *(s + 2) * 0.114);
        d += d_ch;
        s += s_ch;
    }
}
