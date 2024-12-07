#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "core/improc.h"
#include "core/mat.h"

int main(int argc, char **argv) {
    clock_t start = 0;
    clock_t end   = 0;
    int     loop  = 100;

    struct Mat *src = alloc_mat_from_file("../Lenna.png");
    mat_info(src);

    struct Mat *gray_mat = alloc_mat(src->rows, src->cols, CV_8UC1);

    start = clock();
    for (int i = 0; i < loop; i++) {
        cvt_color(gray_mat, src, COLOR_RGB2GRAY);
    }
    end = clock();

    char filename[50];
    strncpy(filename, "Lenna_gray_v1.png", sizeof(filename) - 1);
    mat_write(filename, gray_mat);
    free_mat(&gray_mat);

    printf("Execution time of RGB2GRAY: %lfms \n",
           ((double)(end - start) / CLOCKS_PER_SEC * 1000) / loop);

    sleep(1);

    generate_table();
    struct Mat *gray_mat1 = alloc_mat(src->rows, src->cols, CV_8UC1);

    start = clock();
    for (int i = 0; i < loop; i++) {
        cvt_color(gray_mat1, src, COLOR_RGB2GRAY_INTRIN);
    }
    end = clock();
    strncpy(filename, "Lenna_gray_v2.png", sizeof(filename) - 1);
    mat_write(filename, gray_mat1);
    free_mat(&gray_mat1);

    printf("Execution time of RGB2GRAY: %lfms \n",
           ((double)(end - start) / CLOCKS_PER_SEC * 1000) / loop);
    sleep(1);

    free_mat(&src);
    return 0;
}