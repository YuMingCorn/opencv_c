#include "improc.h"
#include "mat.h"

void generate_table() {
    int i = 0;
    for (; i <= 0xff; i++) {
        table_r[i] = (float)(i * 0.299);
        table_g[i] = (float)(i * 0.587);
        table_b[i] = (float)(i * 0.114);
    }
};

void cvt_color(struct Mat *dst, struct Mat *src, int code) {
    ASSERT(mat_type(src) == CV_8UC3);
    switch (code) {
    case COLOR_RGB2BGR:
        rgb_to_bgr(dst, src);
        break;
    case COLOR_RGB2GRAY_LUT:
        rgb_to_gray_lut(dst, src);
        break;
    case COLOR_RGB2GRAY_THREAD:
        rgb_to_gray_thread(dst, src);
        break;
    case COLOR_RGB2GRAY_INTRIN:
        rgb_to_gray_intinsics(dst,  src);
        break;
    case COLOR_RGB2GRAY:
        rgb_to_gray(dst, src);
        break;
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

void rgb_to_bgr(struct Mat *dst, struct Mat *src) {
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

// void rgb_to_gray(struct Mat* src, struct Mat* dst)
// {
//     for (int i = 0; i < src->rows; i++)
//     {
//         for (int j = 0; j < src->cols; j++)
//         {
//             uchar* s = MAT_AT_ELEM_PTR(*src, uchar, i, j);
//             MAT_AT_ELEM(*dst, uchar, i, j) =
//                 (uchar) (*(s + 0) * 0.299 + *(s + 1) * 0.587 +
//                          *(s + 2) * 0.114);
//         }
//     }
// }

void rgb_to_gray(struct Mat *dst, struct Mat *src) {
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

void *rgb_to_gray_thread_impl(void *args) {
    ThreadData *thread_data = (ThreadData *)args;

    uchar *s = (uchar *)thread_data->src->data;
    uchar *d = (uchar *)thread_data->dst->data;

    size_t s_ch = mat_channel_num(thread_data->src);
    size_t d_ch = mat_channel_num(thread_data->dst);

    for (int i = thread_data->start_row; i < thread_data->end_row; ++i) {
        for (int j = 0; j < thread_data->src->cols; ++j) {
            // *d = (uchar)(*(s + 0) * 0.299 + *(s + 1) * 0.587 + *(s + 2) * 0.114);
            *d  = (uchar)(table_r[*(s)] + table_g[*(s + 1)] + table_b[*(s + 2)]);
            d += d_ch;
            s += s_ch;
        }
    }
    return NULL;
}

void rgb_to_gray_thread(struct Mat *dst, struct Mat *src) {
    const int NUM_THREADS = 4;

    pthread_t  threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    int rows_per_thread = src->rows / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].src       = src;
        thread_data[i].dst       = dst;
        thread_data[i].start_row = i * rows_per_thread;
        thread_data[i].end_row =
            (i == NUM_THREADS - 1) ? src->rows : (i + 1) * rows_per_thread;
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, rgb_to_gray_thread_impl,
                       (void *)&thread_data[i]);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
}

void rgb_to_gray_lut(struct Mat *dst, struct Mat *src) {
    uchar *s     = (uchar *)src->data;
    uchar *d     = (uchar *)dst->data;
    int    total = src->rows * src->cols;
    size_t s_ch  = mat_channel_num(src);
    size_t d_ch  = mat_channel_num(dst);
    for (int i = 0; i < total; i++) {
        *d = (uchar)(table_r[*(s + 0)] + table_g[*(s + 1)] + table_b[*(s + 2)]);
        d += d_ch;
        s += s_ch;
    }
}

void rgb_to_gray_intinsics(struct Mat *dst, struct Mat *src)
{
    uchar *s = (uchar *)src->data;
    uchar *d = (uchar *)dst->data;
    size_t s_ch = mat_channel_num(src);
    size_t d_ch = mat_channel_num(dst);
    int total = src->rows * src->cols;

    #ifdef __ARM_NEON
    uint8x8_t w_r = vdup_n_u8(77);
    uint8x8_t w_g = vdup_n_u8(150);
    uint8x8_t w_b = vdup_n_u8(29);
    for (int i = 0; i <= total - 8; i+= 8){
        uint8x8x3_t rgb = vld3_u8(s);
        s += (s_ch * 8); // (8 * 3) btytes
        uint16x8_t r_weighted = vmull_u8(rgb.val[0], w_r); // R * 77
        uint16x8_t g_weighted = vmlal_u8(r_weighted, rgb.val[1], w_g); // + G * 150
        uint16x8_t b_weighted = vmlal_u8(g_weighted, rgb.val[2], w_b); // + B * 29

        uint8x8_t gray = vshrn_n_u16(b_weighted, 8); // vector shift
        vst1_u8(d, gray);
        d += (d_ch * 8);
    }
    #endif

    #ifdef __SSE__
    ;
    #endif

}

void binary_threshold(struct Mat *dst, struct Mat *src, int thresh,
                      int maxval) {
    uchar *s = (uchar *)src->data;
    uchar *d = (uchar *)dst->data;

    int total = src->rows * src->cols;

    size_t s_ch = mat_channel_num(src);
    size_t d_ch = mat_channel_num(dst);

    for (int i = 0; i < total; i++) {
        *d = THRESH_BINARY(*s, thresh, maxval);
        d += d_ch;
        s += s_ch;
    }
}