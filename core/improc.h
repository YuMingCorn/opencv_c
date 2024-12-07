#ifndef IMPROC_H_
#define IMPROC_H_

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include "mat.h"
#include "utils.h"

#define TABLE_SIZE 256

#define THRESH_BINARY(src, thresh, maxval) ((src) > (thresh) ? (maxval) : 0)

static float table_r[TABLE_SIZE];
static float table_g[TABLE_SIZE];
static float table_b[TABLE_SIZE];

enum ColorConversionCodes {
    COLOR_RGB2GRAY,
    COLOR_RGB2GRAY_LUT,
    COLOR_RGB2GRAY_THREAD,
    COLOR_RGB2GRAY_INTRIN,
    COLOR_RGB2BGR
};

enum ThresholdTypes {
    THRESH_BINARY,
};

typedef struct {
    struct Mat *src;
    struct Mat *dst;
    int         start_row;
    int         end_row;
} ThreadData;

// table
void generate_table();

// interface
void cvt_color(struct Mat *dst, struct Mat *src, int code);
void threshold(struct Mat *dst, struct Mat *src, double thresh, double maxval,
               int type);

// color conversion function
void  rgb_to_bgr(struct Mat *dst, struct Mat *src);
void  rgb_to_gray_lut(struct Mat *dst, struct Mat *src);
void  rgb_to_gray(struct Mat *dst, struct Mat *src);
void  rgb_to_gray_thread(struct Mat *dst, struct Mat *src);
void *rgb_to_gray_thread_impl(void *args);
void rgb_to_gray_intinsics(struct Mat *dst, struct Mat *src);
// threshold function
void binary_threshold(struct Mat *dst, struct Mat *src, int thresh, int maxval);

#endif // IMPROC_H_