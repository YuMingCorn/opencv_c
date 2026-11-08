#include <stdio.h>

#include "improc.h"
#include "mat.h"

#define SYNTHETIC_SIZE 512

/* Usage: demo [input-image] [output-image]
 * Without an input image, a synthetic RGB test pattern is used. */
int main(int argc, char **argv) {
    const char *input  = argc > 1 ? argv[1] : NULL;
    const char *output = argc > 2 ? argv[2] : "gray.jpg";

    struct Mat *src = input
                          ? alloc_mat_from_file(input)
                          : alloc_mat_synthetic(SYNTHETIC_SIZE, SYNTHETIC_SIZE);
    if (src == NULL) {
        fprintf(stderr, "Cannot load input image: %s\n", input);
        return 1;
    }
    mat_info(src);

    struct Mat *gray = alloc_mat(src->rows, src->cols, CV_8UC1);
    cvt_color(gray, src, COLOR_RGB2GRAY);
    mat_write(output, gray);

    free_mat(&gray);
    free_mat(&src);
    return 0;
}
