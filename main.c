#include <stdio.h>

#include "improc.h"

#define SYNTHETIC_SIZE 512

/* Writes <prefix><name>.png and counts a failure instead of aborting. */
static void save(const char *prefix, const char *name, const struct Mat *mat,
                 int *failures) {
    char path[1024];
    snprintf(path, sizeof path, "%s%s.png", prefix, name);
    if (mat_write(path, mat) != 0) {
        fprintf(stderr, "Cannot write %s\n", path);
        (*failures)++;
    }
}

/* Converts any 1, 3 or 4 channel 8-bit image to RGB. Returns NULL if the
 * channel count is not supported. */
static struct Mat *to_rgb(const struct Mat *src) {
    int code;
    switch (CV_MAT_CN(src->flags)) {
    case 1:
        code = COLOR_GRAY2RGB;
        break;
    case 3:
        code = -1;
        break;
    case 4:
        code = COLOR_RGBA2RGB;
        break;
    default:
        return NULL;
    }
    struct Mat *rgb = alloc_mat(src->rows, src->cols, CV_8UC3);
    if (code < 0) {
        for (int r = 0; r < src->rows; r++) {
            uchar       *d = mat_row(rgb, r);
            const uchar *s = mat_row(src, r);
            for (size_t i = 0; i < (size_t)src->cols * 3; i++) {
                d[i] = s[i];
            }
        }
    } else {
        cvt_color(rgb, src, code);
    }
    return rgb;
}

/* Usage: demo [input-image] [output-prefix]
 * Without an input image, a synthetic RGB test pattern is used.
 * Writes 13 PNG files named <output-prefix><step>.png (default prefix out_). */
int main(int argc, char **argv) {
    const char *input  = argc > 1 ? argv[1] : NULL;
    const char *prefix = argc > 2 ? argv[2] : "out_";

    struct Mat *loaded =
        input ? alloc_mat_from_file(input)
              : alloc_mat_synthetic(SYNTHETIC_SIZE, SYNTHETIC_SIZE);
    if (loaded == NULL) {
        fprintf(stderr, "Cannot load input image: %s\n", input);
        return 1;
    }
    mat_info(loaded);

    struct Mat *src = to_rgb(loaded);
    free_mat(&loaded);
    if (src == NULL) {
        fprintf(stderr, "Unsupported channel count in input image\n");
        return 1;
    }

    const int rows      = src->rows;
    const int cols      = src->cols;
    const int half_rows = rows / 2 > 0 ? rows / 2 : 1;
    const int half_cols = cols / 2 > 0 ? cols / 2 : 1;
    int       failures  = 0;

    struct Mat *gray = alloc_mat(rows, cols, CV_8UC1);
    cvt_color(gray, src, COLOR_RGB2GRAY);
    save(prefix, "gray", gray, &failures);

    struct Mat *bgr = alloc_mat(rows, cols, CV_8UC3);
    cvt_color(bgr, src, COLOR_RGB2BGR);
    save(prefix, "bgr", bgr, &failures);

    struct Mat *rgba = alloc_mat(rows, cols, CV_8UC4);
    cvt_color(rgba, src, COLOR_RGB2RGBA);
    save(prefix, "rgba", rgba, &failures);

    struct Mat *out = alloc_mat(rows, cols, CV_8UC3);

    threshold(out, src, 128, 255, THRESH_BINARY);
    save(prefix, "thresh", out, &failures);

    uchar invert[256];
    for (int i = 0; i < 256; i++) {
        invert[i] = (uchar)(255 - i);
    }
    mat_lut(out, src, invert);
    save(prefix, "invert", out, &failures);

    convert_scale(out, src, 384, 10);
    save(prefix, "bright", out, &failures);

    box_filter(out, src, 5);
    save(prefix, "box", out, &failures);

    gaussian_blur(out, src, 7);
    save(prefix, "gauss", out, &failures);

    flip(out, src, FLIP_HORIZONTAL);
    save(prefix, "flip_h", out, &failures);

    struct Mat *grad = alloc_mat(rows, cols, CV_16SC1);
    struct Mat *edge = alloc_mat(rows, cols, CV_8UC1);
    sobel(grad, gray, 1, 0);
    convert_scale_abs(edge, grad, 256, 0);
    save(prefix, "sobel_x", edge, &failures);
    sobel(grad, gray, 0, 1);
    convert_scale_abs(edge, grad, 256, 0);
    save(prefix, "sobel_y", edge, &failures);

    struct Mat *half = alloc_mat(half_rows, half_cols, CV_8UC3);
    resize(half, src, INTER_LINEAR);
    save(prefix, "half", half, &failures);

    struct Mat *twice = alloc_mat(rows * 2, cols * 2, CV_8UC3);
    resize(twice, src, INTER_NEAREST);
    save(prefix, "double_nearest", twice, &failures);

    free_mat(&twice);
    free_mat(&half);
    free_mat(&edge);
    free_mat(&grad);
    free_mat(&out);
    free_mat(&rgba);
    free_mat(&bgr);
    free_mat(&gray);
    free_mat(&src);

    return failures == 0 ? 0 : 1;
}
