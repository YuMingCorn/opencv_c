#include "pixel.h"

static void binary_threshold(struct Mat *dst, struct Mat *src, int thresh,
                             int maxval) {
    uchar *s = (uchar *)src->data;
    uchar *d = (uchar *)dst->data;

    int total = src->rows * src->cols;

    size_t s_ch = mat_channel_num(src);
    size_t d_ch = mat_channel_num(dst);

    for (int i = 0; i < total; i++) {
        *d = (uchar)(*s > thresh ? maxval : 0);
        d += d_ch;
        s += s_ch;
    }
}

void threshold(struct Mat *dst, struct Mat *src, double thresh, double maxval,
               int type) {
    switch (type) {
    case THRESH_BINARY:
        binary_threshold(dst, src, (int)thresh, (int)maxval);
        break;
    }
}
